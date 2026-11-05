#include "weaver_shader.h"

#include "core/object/class_db.h"
#include "servers/rendering/renderer_rd/uniform_set_cache_rd.h"
#include "servers/rendering/rendering_device_binds.h"
#include "servers/rendering/rendering_server.h"


static bool _is_pair_type(RD::UniformType p_type)
{
	return p_type == RD::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE || p_type == RD::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE_BUFFER;
}

static const char *_uniform_type_name(RD::UniformType p_type)
{
	static const char *names[RD::UNIFORM_TYPE_MAX] = {
		"SAMPLER", "SAMPLER_WITH_TEXTURE", "TEXTURE", "IMAGE", "TEXTURE_BUFFER", "SAMPLER_WITH_TEXTURE_BUFFER", "IMAGE_BUFFER",
		"UNIFORM_BUFFER", "STORAGE_BUFFER", "INPUT_ATTACHMENT", "UNIFORM_BUFFER_DYNAMIC", "STORAGE_BUFFER_DYNAMIC", "ACCELERATION_STRUCTURE"
	};
	return p_type >= 0 && p_type < RD::UNIFORM_TYPE_MAX ? names[p_type] : "unknown uniform type";
}

static bool _compatible_types(RD::UniformType p_reflected, RD::UniformType p_bound)
{
	// Reflection cant differentiate between a dynamic buffer and a regular one, so binding a UNIFORM_TYPE_UNIFORM_BUFFER_DYNAMIC variant is allowed.
	return p_reflected == p_bound ||
			(p_reflected == RD::UNIFORM_TYPE_UNIFORM_BUFFER && p_bound == RD::UNIFORM_TYPE_UNIFORM_BUFFER_DYNAMIC) ||
			(p_reflected == RD::UNIFORM_TYPE_STORAGE_BUFFER && p_bound == RD::UNIFORM_TYPE_STORAGE_BUFFER_DYNAMIC);
}

static bool _same_uniforms(const LocalVector<RD::Uniform> &p_a, const LocalVector<RD::Uniform> &p_b)
{
	if (p_a.size() != p_b.size()) return false;

	for (uint32_t i = 0; i < p_a.size(); i++)
	{
		const RD::Uniform &a = p_a[i];
		const RD::Uniform &b = p_b[i];
		if (a.uniform_type != b.uniform_type || a.binding != b.binding || a.get_id_count() != b.get_id_count()) return false;

		for (uint32_t j = 0; j < a.get_id_count(); j++)
		{
			if (a.get_id(j) != b.get_id(j)) return false;
		}
	}
	return true;
}


//
// Setup
//
bool WeaverShader::_setup(const Ref<WeaverRenderContext> &p_context, const Ref<RDShaderFile> &p_file, const StringName &p_version, int64_t p_layout_hash, const Ref<WeaverPushConstants> &p_push_constants)
{
	ERR_FAIL_COND_V_MSG(p_context.is_null(), false, "WeaverShader needs a WeaverRenderContext (WeaverWebNode.get_render_context()).");
	ERR_FAIL_COND_V(p_file.is_null(), false);
	const String name = p_file->get_path().is_empty() ? String("<unsaved shader>") : p_file->get_path();

	// Reflect first: an outdated class must not compile anything.
	const Ref<WeaverShaderReflection> reflected = WeaverShaderReflection::reflect(p_file, p_version);
	ERR_FAIL_COND_V_MSG(reflected.is_null(), false, vformat("%s could not be reflected (see the error above).", name));
	ERR_FAIL_COND_V_MSG(reflected->get_layout_hash() != p_layout_hash, false,
			vformat("%s changed since its WeaverShader class was generated (LAYOUT_HASH %d, shader %d). Regenerate the class: save the shader in the editor.",
					name, p_layout_hash, reflected->get_layout_hash()));

	const RID shader = p_context->get_shader_rid(p_file, p_version);
	ERR_FAIL_COND_V(!shader.is_valid(), false);

	context = p_context;
	file = p_file;
	version = p_version;
	reflection = reflected;
	rid = shader;
	path = name;

	expected.clear();
	expected_index.clear();
	used_sets.clear();
	local_sets.clear();
	const TypedArray<Dictionary> bindings = reflection->get_bindings();
	for (int i = 0; i < bindings.size(); i++)
	{
		const Dictionary b = bindings[i];
		ExpectedBinding e;
		e.set = int(b["set"]);
		e.binding = int(b["binding"]);
		e.type = RD::UniformType(int(b["uniform_type"]));
		e.count = int(b["count"]);
		e.name = b["name"];
		expected.push_back(e);
	}
	expected.sort_custom<ExpectedOrder>();
	for (uint32_t i = 0; i < expected.size(); i++)
	{
		expected_index.insert(_key(expected[i].set, expected[i].binding), i);
		if (used_sets.is_empty() || used_sets[used_sets.size() - 1] != int(expected[i].set))
		{
			used_sets.push_back(expected[i].set);
		}
	}

	// Bindings set before _setup() are checked now.
	LocalVector<uint64_t> invalid;
	for (const KeyValue<uint64_t, BoundUniform> &E : bound)
	{
		const uint32_t *index = expected_index.getptr(E.key);
		if (index == nullptr)
		{
			ERR_PRINT(vformat("%s has no binding %d in set %d.", path, uint32_t(E.key), uint32_t(E.key >> 32)));
			invalid.push_back(E.key);
		}
		else if (!_compatible_types(expected[*index].type, E.value.type))
		{
			ERR_PRINT(vformat("%s: \"%s\" (set %d, binding %d) is a %s, not a %s.", path, expected[*index].name, expected[*index].set, expected[*index].binding,
					_uniform_type_name(expected[*index].type), _uniform_type_name(E.value.type)));
			invalid.push_back(E.key);
		}
	}
	for (uint64_t key : invalid)
	{
		bound.erase(key);
	}

	push_constants = p_push_constants;
	const int block_size = reflection->get_push_constant_size();
	if (push_constants.is_valid())
	{
		ERR_FAIL_COND_V_MSG(block_size == 0, false, vformat("%s has no push constants, but the class has a push constant block. Regenerate the class.", path));
		push_constants->set_block_size(block_size);

		const TypedArray<Dictionary> members = reflection->get_push_constant_members();
		for (int i = 0; i < members.size(); i++)
		{
			const Dictionary m = members[i];
			PushConstantMember member;
			member.offset = m["offset"];
			member.name = m["name"];
		}
	}
	return true;
}


//
// Checks
//
bool WeaverShader::_check_ready(const char *p_function) const
{
	ERR_FAIL_COND_V_MSG(reflection.is_null(), false, vformat("WeaverShader.%s(): the object was not set up. Create it with the generated create().", p_function));
	return true;
}

bool WeaverShader::_check_device(RenderingDevice *p_rd) const
{
	ERR_FAIL_NULL_V_MSG(p_rd, false, vformat("%s: the RenderingDevice passed is null.", path));
	ERR_FAIL_COND_V_MSG(p_rd != context->get_device(), false, vformat("%s: the RenderingDevice passed is not the device of the WeaverRenderContext this shader was created with. The shader only exists on that device.", path));
	return true;
}

bool WeaverShader::_check_binding(int p_set, int p_binding, RD::UniformType p_type) const
{
	ERR_FAIL_COND_V(p_set < 0 || p_binding < 0, false);
	if (reflection.is_null()) return true; // Checked in _setup().

	const uint32_t *index = expected_index.getptr(_key(p_set, p_binding));
	ERR_FAIL_NULL_V_MSG(index, false, vformat("%s has no binding %d in set %d.", path, p_binding, p_set));

	const ExpectedBinding &e = expected[*index];
	ERR_FAIL_COND_V_MSG(!_compatible_types(e.type, p_type), false, vformat("%s: \"%s\" (set %d, binding %d) is a %s, not a %s.", path, e.name, p_set, p_binding, _uniform_type_name(e.type), _uniform_type_name(p_type)));

	return true;
}


//
// Bindings
//
void WeaverShader::_store(int p_set, int p_binding, RD::UniformType p_type, const Vector<RID> &p_ids)
{
	if (p_ids.is_empty())
	{
		bound.erase(_key(p_set, p_binding));
	}
	else
	{
		bound[_key(p_set, p_binding)] = { p_type, p_ids };
	}
}

void WeaverShader::_bind(int p_set, int p_binding, RD::UniformType p_type, const RID &p_rid)
{
	ERR_FAIL_COND_MSG(_is_pair_type(p_type), "Combined samplers take a sampler and a texture: use _bind_pair().");
	if (!_check_binding(p_set, p_binding, p_type)) return;

	Vector<RID> ids;
	if (p_rid.is_valid())
	{
		ids.push_back(p_rid);
	}

	_store(p_set, p_binding, p_type, ids);
}

void WeaverShader::_bind_pair(int p_set, int p_binding, RD::UniformType p_type, const RID &p_sampler, const RID &p_texture)
{
	ERR_FAIL_COND_MSG(!_is_pair_type(p_type), "_bind_pair() is only for combined samplers (UNIFORM_TYPE_SAMPLER_WITH_TEXTURE*).");
	if (!_check_binding(p_set, p_binding, p_type)) return;

	Vector<RID> ids;
	if (p_sampler.is_valid() && p_texture.is_valid())
	{
		ids.push_back(p_sampler);
		ids.push_back(p_texture);
	}

	_store(p_set, p_binding, p_type, ids);
}

void WeaverShader::_bind_array(int p_set, int p_binding, RD::UniformType p_type, const Array &p_rids)
{
	ERR_FAIL_COND_MSG(_is_pair_type(p_type), "Combined sampler arrays take samplers and textures: use _bind_pair_array().");
	if (!_check_binding(p_set, p_binding, p_type)) return;

	Vector<RID> ids;
	for (int i = 0; i < p_rids.size(); i++)
	{
		ERR_FAIL_COND_MSG(p_rids[i].get_type() != Variant::RID, vformat("%s: element %d bound to set %d, binding %d is not a RID.", path, i, p_set, p_binding));
		ids.push_back(p_rids[i]);
	}

	_store(p_set, p_binding, p_type, ids);
}

void WeaverShader::_bind_pair_array(int p_set, int p_binding, RD::UniformType p_type, const Array &p_samplers, const Array &p_textures)
{
	ERR_FAIL_COND_MSG(!_is_pair_type(p_type), "_bind_pair_array() is only for combined samplers (UNIFORM_TYPE_SAMPLER_WITH_TEXTURE*).");
	if (!_check_binding(p_set, p_binding, p_type)) return;
	ERR_FAIL_COND_MSG(p_samplers.size() != 1 && p_samplers.size() != p_textures.size(), vformat("%s: set %d, binding %d needs one sampler, or one per texture (%d samplers, %d textures).", path, p_set, p_binding, p_samplers.size(), p_textures.size()));

	Vector<RID> ids;
	for (int i = 0; i < p_textures.size(); i++)
	{
		const Variant &sampler = p_samplers[p_samplers.size() == 1 ? 0 : i];
		ERR_FAIL_COND_MSG(sampler.get_type() != Variant::RID || p_textures[i].get_type() != Variant::RID,
				vformat("%s: element %d bound to set %d, binding %d is not a RID.", path, i, p_set, p_binding));
		ids.push_back(sampler);
		ids.push_back(p_textures[i]);
	}

	_store(p_set, p_binding, p_type, ids);
}


//
// Uniform sets
//
RID WeaverShader::_get_uniform_set(RenderingDevice *p_rd, uint32_t p_set, const LocalVector<RD::Uniform> &p_uniforms)
{
	UniformSetCacheRD *cache = UniformSetCacheRD::get_singleton();
	if (cache != nullptr && p_rd == RenderingServer::get_singleton()->get_rendering_device())
	{
		return cache->get_cache_vec(rid, p_set, p_uniforms);
	}

	// UniformSetCacheRD only knows the main device.
	LocalUniformSet *local = local_sets.getptr(p_set);
	if (local != nullptr && local->rid.is_valid() && p_rd->uniform_set_is_valid(local->rid))
	{
		if (_same_uniforms(local->uniforms, p_uniforms)) return local->rid;
		p_rd->free_rid(local->rid);
	}

	const RID uniform_set = p_rd->uniform_set_create(p_uniforms, rid, p_set);
	ERR_FAIL_COND_V(!uniform_set.is_valid(), RID());

	LocalUniformSet &stored = local_sets[p_set];
	stored.uniforms = p_uniforms;
	stored.rid = uniform_set;
	return uniform_set;
}

Error WeaverShader::bind_uniforms(RenderingDevice *p_rd, int64_t p_list, const PackedInt32Array &p_sets)
{
	if (!_check_ready("bind_uniforms")) return ERR_UNCONFIGURED;
	if (!_check_device(p_rd)) return ERR_INVALID_PARAMETER;

	Error result = OK;
	const PackedInt32Array &sets = p_sets.is_empty() ? used_sets : p_sets;
	for (int set : sets)
	{
		if (!used_sets.has(set))
		{
			ERR_PRINT(vformat("%s has no uniform set %d.", path, set));
			result = ERR_INVALID_PARAMETER;
			continue;
		}

		LocalVector<RD::Uniform> uniforms;
		bool complete = true;
		for (const ExpectedBinding &e : expected)
		{
			if (int(e.set) != set) continue;

			const BoundUniform *b = bound.getptr(_key(e.set, e.binding));
			if (b == nullptr)
			{
				ERR_PRINT(vformat("%s: \"%s\" (set %d, binding %d) was never set. Uniform set %d is not bound.", path, e.name, e.set, e.binding, set));
				complete = false;
				continue;
			}

			const uint32_t needed = e.count * (_is_pair_type(e.type) ? 2 : 1);
			const Vector<RID> &ids = b->ids;
			if (e.count > 0 && uint32_t(ids.size()) != needed)
			{
				ERR_PRINT(vformat("%s: \"%s\" (set %d, binding %d) is an array of %d, but %d were set. Uniform set %d is not bound.",
						path, e.name, e.set, e.binding, e.count, _is_pair_type(e.type) ? ids.size() / 2 : ids.size(), set));
				complete = false;
				continue;
			}

			RD::Uniform uniform;
			uniform.uniform_type = b->type;
			uniform.binding = e.binding;
			for (const RID &id : ids)
			{
				uniform.append_id(id);
			}
			uniforms.push_back(uniform);
		}
		if (!complete)
		{
			result = ERR_UNCONFIGURED;
			continue;
		}

		const RID uniform_set = _get_uniform_set(p_rd, set, uniforms);
		if (!uniform_set.is_valid())
		{
			result = FAILED;
			continue;
		}
		if (reflection->is_compute())
		{
			p_rd->compute_list_bind_uniform_set(p_list, uniform_set, set);
		}
		else
		{
			p_rd->draw_list_bind_uniform_set(p_list, uniform_set, set);
		}
	}
	return result;
}


//
// Push constants
//
Error WeaverShader::set_push_constants(RenderingDevice *p_rd, int64_t p_list)
{
	if (!_check_ready("set_push_constants")) return ERR_UNCONFIGURED;
	if (!_check_device(p_rd)) return ERR_INVALID_PARAMETER;
	const int size = reflection->get_push_constant_size();
	ERR_FAIL_COND_V_MSG(size == 0, ERR_UNAVAILABLE, vformat("%s has no push constants.", path));
	ERR_FAIL_COND_V_MSG(push_constants.is_null(), ERR_UNCONFIGURED, vformat("%s: the object has no push constant block. Regenerate the class.", path));

	const Vector<uint8_t> &data = push_constants->get_data_ref();
	ERR_FAIL_COND_V(data.size() != size, FAILED);

	if (reflection->is_compute())
	{
		p_rd->compute_list_set_push_constant(p_list, data.ptr(), size);
	}
	else
	{
		p_rd->draw_list_set_push_constant(p_list, data.ptr(), size);
	}

	return OK;
}


Vector3i WeaverShader::groups_for(const Vector3i &p_threads) const
{
	if (!_check_ready("groups_for")) return Vector3i();

	ERR_FAIL_COND_V_MSG(!reflection->is_compute(), Vector3i(), vformat("%s is not a compute shader.", path));

	const Vector3i local = reflection->get_local_size();
	ERR_FAIL_COND_V(local.x <= 0 || local.y <= 0 || local.z <= 0, Vector3i());

	return Vector3i(
			(MAX(p_threads.x, 0) + local.x - 1) / local.x,
			(MAX(p_threads.y, 0) + local.y - 1) / local.y,
			(MAX(p_threads.z, 0) + local.z - 1) / local.z
	);
}


void WeaverShader::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_rid"), &WeaverShader::get_rid);
	ClassDB::bind_method(D_METHOD("get_context"), &WeaverShader::get_context);
	ClassDB::bind_method(D_METHOD("get_file"), &WeaverShader::get_file);
	ClassDB::bind_method(D_METHOD("get_version"), &WeaverShader::get_version);
	ClassDB::bind_method(D_METHOD("get_reflection"), &WeaverShader::get_reflection);

	ClassDB::bind_method(D_METHOD("bind_uniforms", "rd", "list", "sets"), &WeaverShader::bind_uniforms, DEFVAL(PackedInt32Array()));
	ClassDB::bind_method(D_METHOD("set_push_constants", "rd", "list"), &WeaverShader::set_push_constants);
	ClassDB::bind_method(D_METHOD("groups_for", "threads"), &WeaverShader::groups_for);

	ClassDB::bind_method(D_METHOD("_setup", "context", "file", "version", "layout_hash", "push_constants"), &WeaverShader::_setup);
	ClassDB::bind_method(D_METHOD("_bind", "set", "binding", "uniform_type", "rid"), &WeaverShader::_bind);
	ClassDB::bind_method(D_METHOD("_bind_pair", "set", "binding", "uniform_type", "sampler", "texture"), &WeaverShader::_bind_pair);
	ClassDB::bind_method(D_METHOD("_bind_array", "set", "binding", "uniform_type", "rids"), &WeaverShader::_bind_array);
	ClassDB::bind_method(D_METHOD("_bind_pair_array", "set", "binding", "uniform_type", "samplers", "textures"), &WeaverShader::_bind_pair_array);

	ADD_PROPERTY(PropertyInfo(Variant::RID, "rid"), "", "get_rid");
}
