#include "weaver_shader_reflection.h"

#include "core/object/class_db.h"
#include "servers/rendering/rendering_device_binds.h"

#include <thirdparty/spirv-reflect/spirv_reflect.h>


namespace {

struct ReflectedBinding
{
	String name; 
	String block_name;
	uint32_t set = 0;
	uint32_t binding = 0;
	RD::UniformType type = RD::UNIFORM_TYPE_MAX;
	uint32_t count = 1;
	bool writable = false;
	uint32_t stages = 0; // Bitmask of RD::ShaderStage.
	RD::ShaderStage first_stage = RD::SHADER_STAGE_MAX;
};

struct ReflectedMember
{
	String name; 
	uint32_t offset = 0;
	uint32_t size = 0;
	WeaverShaderReflection::ScalarType scalar = WeaverShaderReflection::SCALAR_UNSUPPORTED;
	uint32_t components = 1; // Rows for matrices.
	uint32_t columns = 1;
	uint32_t column_stride = 0;
	bool row_major = false;
	uint32_t array_count = 0; // 0 when not an array.
	uint32_t array_stride = 0;
	String glsl_type;

	bool operator==(const ReflectedMember &p_other) const
	{
		return name == p_other.name && offset == p_other.offset && size == p_other.size && glsl_type == p_other.glsl_type &&
				column_stride == p_other.column_stride && row_major == p_other.row_major && array_stride == p_other.array_stride;
	}
};

struct ReflectedConstant
{
	String name;
	uint32_t constant_id = 0;
	RD::PipelineSpecializationConstantType type = RD::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_INT;
	Variant default_value;
	uint32_t stages = 0;
	RD::ShaderStage first_stage = RD::SHADER_STAGE_MAX;
};

struct BindingOrder
{
	bool operator()(const ReflectedBinding &a, const ReflectedBinding &b) const { return a.set != b.set ? a.set < b.set : a.binding < b.binding; }
};

struct ConstantOrder
{
	bool operator()(const ReflectedConstant &a, const ReflectedConstant &b) const { return a.constant_id < b.constant_id; }
};

bool _same_members(const LocalVector<ReflectedMember> &a, const LocalVector<ReflectedMember> &b)
{
	if (a.size() != b.size()) return false;
	for (uint32_t i = 0; i < a.size(); i++)
	{
		if (!(a[i] == b[i])) return false;
	}
	return true;
}

// Destroys the spirv-reflect module when leaving the scope, also on the early error returns.
struct ModuleScope
{
	SpvReflectShaderModule module = {};
	bool valid = false;
	~ModuleScope()
	{
		if (valid) spvReflectDestroyShaderModule(&module);
	}
};

String _stage_name(RD::ShaderStage p_stage)
{
	return p_stage < RD::SHADER_STAGE_MAX ? String(RD::SHADER_STAGE_NAMES[p_stage]) : String("?");
}

String _binding_description(const ReflectedBinding &p_binding)
{
	static const char *type_names[RD::UNIFORM_TYPE_MAX] = {
		"sampler", "sampler with texture", "texture", "image", "texture buffer", "sampler with texture buffer", "image buffer",
		"uniform buffer", "storage buffer", "input attachment", "uniform buffer dynamic", "storage buffer dynamic", "acceleration structure",
	};
	const String type = p_binding.type < RD::UNIFORM_TYPE_MAX ? String(type_names[p_binding.type]) : itos(p_binding.type);
	String description = vformat("\"%s\" (%s", p_binding.name, type);
	if (p_binding.count != 1) description += vformat(", %d elements", p_binding.count);
	if (p_binding.writable) description += ", writable";
	return description + ")";
}

String _glsl_type(const ReflectedMember &p_member)
{
	String type;
	const bool is_float = p_member.scalar == WeaverShaderReflection::SCALAR_FLOAT;
	if (p_member.scalar == WeaverShaderReflection::SCALAR_UNSUPPORTED || (p_member.columns > 1 && !is_float))
	{
		type = "unsupported";
	}
	else if (p_member.columns > 1)
	{
		type = p_member.columns == p_member.components ? vformat("mat%d", p_member.columns) : vformat("mat%dx%d", p_member.columns, p_member.components);
	}
	else
	{
		static const char *prefixes[] = { "", "i", "u", "b" };
		static const char *scalars[] = { "float", "int", "uint", "bool" };
		type = p_member.components > 1 ? vformat("%svec%d", prefixes[p_member.scalar], p_member.components) : String(scalars[p_member.scalar]);
	}

	if (p_member.array_count > 0)
	{
		type += vformat("[%d]", p_member.array_count);
	}
	return type;
}

// Push constant members, nested structs flattened. Member offsets are relative to their parent struct.
void _flatten_members(const SpvReflectBlockVariable &p_block, const String &p_prefix, uint32_t p_base_offset, LocalVector<ReflectedMember> &r_members)
{
	for (uint32_t i = 0; i < p_block.member_count; i++)
	{
		const SpvReflectBlockVariable &member = p_block.members[i];
		const SpvReflectTypeDescription *type = member.type_description;
		const uint32_t flags = type != nullptr ? type->type_flags : 0;
		const bool is_array = flags & SPV_REFLECT_TYPE_FLAG_ARRAY;
		const String name = p_prefix + String::utf8(member.name != nullptr ? member.name : "");
		const uint32_t offset = p_base_offset + member.offset;

		if ((flags & SPV_REFLECT_TYPE_FLAG_STRUCT) && !is_array)
		{
			_flatten_members(member, name + ".", offset, r_members);
			continue;
		}

		ReflectedMember m;
		m.name = name;
		m.offset = offset;
		m.size = member.size;

		if (is_array)
		{
			m.array_count = 1;
			for (uint32_t d = 0; d < member.array.dims_count; d++)
			{
				m.array_count *= member.array.dims[d];
			}
			m.array_stride = member.array.stride;
		}

		if (type == nullptr || (flags & SPV_REFLECT_TYPE_FLAG_STRUCT) || type->traits.numeric.scalar.width != 32)
		{
			m.scalar = WeaverShaderReflection::SCALAR_UNSUPPORTED;
		}
		else if (flags & SPV_REFLECT_TYPE_FLAG_FLOAT)
		{
			m.scalar = WeaverShaderReflection::SCALAR_FLOAT;
		}
		else if (flags & SPV_REFLECT_TYPE_FLAG_INT)
		{
			m.scalar = type->traits.numeric.scalar.signedness ? WeaverShaderReflection::SCALAR_INT : WeaverShaderReflection::SCALAR_UINT;
		}
		else if (flags & SPV_REFLECT_TYPE_FLAG_BOOL)
		{
			m.scalar = WeaverShaderReflection::SCALAR_BOOL;
		}

		if (type != nullptr && (flags & SPV_REFLECT_TYPE_FLAG_MATRIX))
		{
			m.columns = type->traits.numeric.matrix.column_count;
			m.components = type->traits.numeric.matrix.row_count;
			m.column_stride = member.numeric.matrix.stride != 0 ? member.numeric.matrix.stride : type->traits.numeric.matrix.stride;
			m.row_major = member.decoration_flags & SPV_REFLECT_DECORATION_ROW_MAJOR;
		}
		else if (type != nullptr && (flags & SPV_REFLECT_TYPE_FLAG_VECTOR))
		{
			m.components = type->traits.numeric.vector.component_count;
		}

		m.glsl_type = _glsl_type(m);
		r_members.push_back(m);
	}
}

bool _uniform_type(const SpvReflectDescriptorBinding &p_binding, RD::UniformType &r_type, bool &r_may_be_writable)
{
	r_may_be_writable = false;
	switch (p_binding.descriptor_type)
	{
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: r_type = RD::UNIFORM_TYPE_SAMPLER; return true;
		case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: r_type = RD::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE; return true;
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE: r_type = RD::UNIFORM_TYPE_TEXTURE; return true;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: r_type = RD::UNIFORM_TYPE_IMAGE; r_may_be_writable = true; return true;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: r_type = RD::UNIFORM_TYPE_TEXTURE_BUFFER; return true;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: r_type = RD::UNIFORM_TYPE_IMAGE_BUFFER; r_may_be_writable = true; return true;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER: r_type = RD::UNIFORM_TYPE_UNIFORM_BUFFER; return true;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: r_type = RD::UNIFORM_TYPE_STORAGE_BUFFER; r_may_be_writable = true; return true;
		case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: r_type = RD::UNIFORM_TYPE_INPUT_ATTACHMENT; return true;
		case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: r_type = RD::UNIFORM_TYPE_ACCELERATION_STRUCTURE; return true;
		default: return false; // Dynamic buffers only come from the engine's own shaders.
	}
}

} // namespace


Ref<WeaverShaderReflection> WeaverShaderReflection::reflect(const Ref<RDShaderFile> &p_file, const StringName &p_version)
{
	ERR_FAIL_COND_V(p_file.is_null(), Ref<WeaverShaderReflection>());

	const String name = p_file->get_path().is_empty() ? String("<unsaved shader>") : p_file->get_path();
	ERR_FAIL_COND_V_MSG(!p_file->get_base_error().is_empty(), Ref<WeaverShaderReflection>(), vformat("%s has errors and can't be reflected.", name));
	ERR_FAIL_COND_V_MSG(!p_file->get_version_list().has(p_version), Ref<WeaverShaderReflection>(), vformat("%s has no version \"%s\".", name, p_version));

	const Ref<RDShaderSPIRV> spirv = p_file->get_spirv(p_version);
	ERR_FAIL_COND_V(spirv.is_null(), Ref<WeaverShaderReflection>());

	LocalVector<ReflectedBinding> bindings;
	LocalVector<ReflectedConstant> constants;
	LocalVector<ReflectedMember> members;
	uint32_t push_constant_size = 0;
	RD::ShaderStage push_constant_stage = RD::SHADER_STAGE_MAX;
	PackedInt32Array stages;
	bool has_compute = false;
	bool has_graphics = false;
	Vector3i local_size;
	int color_output_count = 0;

	for (int s = 0; s < RD::SHADER_STAGE_MAX; s++)
	{
		const RD::ShaderStage stage = RD::ShaderStage(s);
		const Vector<uint8_t> bytecode = spirv->get_stage_bytecode(stage);
		if (bytecode.is_empty()) continue;

		const String stage_name = _stage_name(stage);
		ERR_FAIL_COND_V_MSG(!spirv->get_stage_compile_error(stage).is_empty(), Ref<WeaverShaderReflection>(), vformat("%s: the %s stage failed to compile.", name, stage_name));

		ModuleScope scope;
		ERR_FAIL_COND_V_MSG(spvReflectCreateShaderModule2(SPV_REFLECT_MODULE_FLAG_NO_COPY, bytecode.size(), bytecode.ptr(), &scope.module) != SPV_REFLECT_RESULT_SUCCESS,
				Ref<WeaverShaderReflection>(), vformat("%s: the %s stage could not be parsed.", name, stage_name));
		scope.valid = true;
		const SpvReflectShaderModule &module = scope.module;

		stages.push_back(stage);
		if (stage == RD::SHADER_STAGE_COMPUTE)
		{
			has_compute = true;
			if (module.entry_point_count > 0)
			{
				local_size = Vector3i(module.entry_points[0].local_size.x, module.entry_points[0].local_size.y, module.entry_points[0].local_size.z);
			}
		}
		else
		{
			has_graphics = true;
		}
		ERR_FAIL_COND_V_MSG(has_compute && has_graphics, Ref<WeaverShaderReflection>(), vformat("%s mixes a compute stage with other stages.", name));

		//
		// Bindings
		//
		uint32_t binding_count = 0;
		spvReflectEnumerateDescriptorBindings(&module, &binding_count, nullptr);
		LocalVector<SpvReflectDescriptorBinding *> stage_bindings;
		stage_bindings.resize(binding_count);
		if (binding_count > 0)
		{
			spvReflectEnumerateDescriptorBindings(&module, &binding_count, stage_bindings.ptr());
		}

		for (const SpvReflectDescriptorBinding *spv_binding : stage_bindings)
		{
			ReflectedBinding b;
			b.block_name = (spv_binding->type_description != nullptr && spv_binding->type_description->type_name != nullptr) ? String::utf8(spv_binding->type_description->type_name) : String();
			b.name = spv_binding->name != nullptr ? String::utf8(spv_binding->name) : String();
			if (b.name.is_empty()) b.name = b.block_name;
			b.set = spv_binding->set;
			b.binding = spv_binding->binding;
			b.count = spv_binding->count;
			b.stages = 1 << stage;
			b.first_stage = stage;

			ERR_FAIL_COND_V_MSG(b.name.is_empty(), Ref<WeaverShaderReflection>(),
					vformat("%s: the binding at set %d, binding %d has no name. The SPIR-V was stripped of its debug names, it can't be reflected.", name, b.set, b.binding));

			bool may_be_writable = false;
			ERR_FAIL_COND_V_MSG(!_uniform_type(*spv_binding, b.type, may_be_writable), Ref<WeaverShaderReflection>(),
					vformat("%s: \"%s\" (%s stage) uses an unsupported descriptor type.", name, b.name, stage_name));
			if (may_be_writable)
			{
				b.writable = !(spv_binding->decoration_flags & SPV_REFLECT_DECORATION_NON_WRITABLE);
				if (spv_binding->descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE)
				{
					b.writable = b.writable && !(spv_binding->block.decoration_flags & SPV_REFLECT_DECORATION_NON_WRITABLE);
				}
			}

			bool merged = false;
			for (ReflectedBinding &existing : bindings)
			{
				if (existing.set == b.set && existing.binding == b.binding)
				{
					ERR_FAIL_COND_V_MSG(existing.name != b.name || existing.type != b.type || existing.count != b.count || existing.writable != b.writable, Ref<WeaverShaderReflection>(),
							vformat("%s: set %d, binding %d is %s in the %s stage but %s in the %s stage. It must be the same in every stage.",
									name, b.set, b.binding, _binding_description(existing), _stage_name(existing.first_stage), _binding_description(b), stage_name));
					existing.stages |= b.stages;
					merged = true;
					break;
				}
				ERR_FAIL_COND_V_MSG(existing.name == b.name, Ref<WeaverShaderReflection>(),
						vformat("%s: \"%s\" is used by set %d, binding %d (%s stage) and by set %d, binding %d (%s stage). Binding names must be unique.",
								name, b.name, existing.set, existing.binding, _stage_name(existing.first_stage), b.set, b.binding, stage_name));
			}
			if (!merged) bindings.push_back(b);
		}

		//
		// Specialization constants
		//
		uint32_t constant_count = 0;
		spvReflectEnumerateSpecializationConstants(&module, &constant_count, nullptr);
		LocalVector<SpvReflectSpecializationConstant *> stage_constants;
		stage_constants.resize(constant_count);
		if (constant_count > 0)
		{
			spvReflectEnumerateSpecializationConstants(&module, &constant_count, stage_constants.ptr());
		}

		for (const SpvReflectSpecializationConstant *spv_constant : stage_constants)
		{
			ReflectedConstant c;
			c.name = spv_constant->name != nullptr ? String::utf8(spv_constant->name) : String();
			c.constant_id = spv_constant->constant_id;
			c.stages = 1 << stage;
			c.first_stage = stage;

			ERR_FAIL_COND_V_MSG(c.name.is_empty(), Ref<WeaverShaderReflection>(),
					vformat("%s: the specialization constant %d has no name. The SPIR-V was stripped of its debug names, it can't be reflected.", name, c.constant_id));
			ERR_FAIL_COND_V_MSG(spv_constant->default_value_size != 4 || spv_constant->type_description == nullptr, Ref<WeaverShaderReflection>(),
					vformat("%s: the specialization constant \"%s\" is not 32 bits, only bool, int, uint and float are supported.", name, c.name));

			const uint32_t raw = *(const uint32_t *)spv_constant->default_value;
			switch (spv_constant->type_description->op)
			{
				case SpvOpTypeBool:
					c.type = RD::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_BOOL;
					c.default_value = raw != 0;
					break;
				case SpvOpTypeInt:
					c.type = RD::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_INT;
					c.default_value = spv_constant->type_description->traits.numeric.scalar.signedness ? int64_t(int32_t(raw)) : int64_t(raw);
					break;
				case SpvOpTypeFloat:
					c.type = RD::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_FLOAT;
					c.default_value = *(const float *)spv_constant->default_value;
					break;
				default:
					ERR_FAIL_V_MSG(Ref<WeaverShaderReflection>(), vformat("%s: the specialization constant \"%s\" has an unsupported type.", name, c.name));
			}

			bool merged = false;
			for (ReflectedConstant &existing : constants)
			{
				if (existing.constant_id == c.constant_id)
				{
					ERR_FAIL_COND_V_MSG(existing.name != c.name || existing.type != c.type || existing.default_value != c.default_value, Ref<WeaverShaderReflection>(),
							vformat("%s: specialization constant %d is \"%s\" in the %s stage and differs in the %s stage. It must be the same in every stage.",
									name, c.constant_id, existing.name, _stage_name(existing.first_stage), stage_name));
					existing.stages |= c.stages;
					merged = true;
					break;
				}
			}
			if (!merged) constants.push_back(c);
		}

		//
		// Push constants
		//
		uint32_t block_count = 0;
		spvReflectEnumeratePushConstantBlocks(&module, &block_count, nullptr);
		ERR_FAIL_COND_V_MSG(block_count > 1, Ref<WeaverShaderReflection>(), vformat("%s: the %s stage has more than one push constant block.", name, stage_name));
		if (block_count == 1)
		{
			SpvReflectBlockVariable *block = nullptr;
			spvReflectEnumeratePushConstantBlocks(&module, &block_count, &block);

			LocalVector<ReflectedMember> stage_members;
			_flatten_members(*block, String(), 0, stage_members);
			for (const ReflectedMember &m : stage_members)
			{
				ERR_FAIL_COND_V_MSG(m.name.is_empty() || m.name.ends_with("."), Ref<WeaverShaderReflection>(),
						vformat("%s: a push constant member at offset %d has no name. The SPIR-V was stripped of its debug names, it can't be reflected.", name, m.offset));
			}

			if (push_constant_stage == RD::SHADER_STAGE_MAX)
			{
				members = stage_members;
				push_constant_size = block->size;
				push_constant_stage = stage;
			}
			else
			{
				ERR_FAIL_COND_V_MSG(push_constant_size != block->size || !_same_members(members, stage_members), Ref<WeaverShaderReflection>(),
						vformat("%s: the push constant block of the %s stage differs from the one of the %s stage. It must be the same in every stage.",
								name, stage_name, _stage_name(push_constant_stage)));
			}
		}

		//
		// Fragment outputs
		//
		if (stage == RD::SHADER_STAGE_FRAGMENT)
		{
			uint32_t output_count = 0;
			spvReflectEnumerateOutputVariables(&module, &output_count, nullptr);
			LocalVector<SpvReflectInterfaceVariable *> outputs;
			outputs.resize(output_count);
			if (output_count > 0)
			{
				spvReflectEnumerateOutputVariables(&module, &output_count, outputs.ptr());
			}
			for (const SpvReflectInterfaceVariable *output : outputs)
			{
				if (output != nullptr && !(output->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN))
				{
					color_output_count++;
				}
			}
		}
	}

	ERR_FAIL_COND_V_MSG(stages.is_empty(), Ref<WeaverShaderReflection>(), vformat("%s has no compiled stage.", name));

	bindings.sort_custom<BindingOrder>();
	constants.sort_custom<ConstantOrder>();

	//
	// Result and layout hash
	//
	Ref<WeaverShaderReflection> result;
	result.instantiate();
	result->shader_name = name;
	result->stages = stages;
	result->compute = has_compute;
	result->push_constant_size = push_constant_size;
	result->local_size = local_size;
	result->color_output_count = color_output_count;

	String hashed = has_compute ? "compute;" : "graphics;";

	for (const ReflectedBinding &b : bindings)
	{
		Dictionary d;
		d["name"] = b.name;
		d["block_name"] = b.block_name;
		d["set"] = b.set;
		d["binding"] = b.binding;
		d["uniform_type"] = b.type;
		d["count"] = b.count;
		d["writable"] = b.writable;
		d["stages"] = b.stages;
		result->bindings.push_back(d);
		hashed += vformat("b:%s:%d:%d:%d:%d:%d;", b.name, b.set, b.binding, b.type, b.count, int(b.writable));
	}

	hashed += vformat("pc:%d;", push_constant_size);
	for (const ReflectedMember &m : members)
	{
		Dictionary d;
		d["name"] = m.name;
		d["offset"] = m.offset;
		d["size"] = m.size;
		d["scalar"] = m.scalar;
		d["components"] = m.components;
		d["columns"] = m.columns;
		d["column_stride"] = m.column_stride;
		d["row_major"] = m.row_major;
		d["array_count"] = m.array_count;
		d["array_stride"] = m.array_stride;
		d["glsl_type"] = m.glsl_type;
		result->push_constant_members.push_back(d);
		hashed += vformat("m:%s:%d:%d:%s:%d:%d:%d;", m.name, m.offset, m.size, m.glsl_type, m.column_stride, int(m.row_major), m.array_stride);
	}

	for (const ReflectedConstant &c : constants)
	{
		Dictionary d;
		d["name"] = c.name;
		d["constant_id"] = c.constant_id;
		d["type"] = c.type;
		d["default_value"] = c.default_value;
		d["stages"] = c.stages;
		result->specialization_constants.push_back(d);
		hashed += vformat("s:%s:%d:%d:%s;", c.name, c.constant_id, c.type, c.default_value);
	}

	result->layout_hash = int64_t(hashed.hash64());
	return result;
}


void WeaverShaderReflection::_bind_methods()
{
	ClassDB::bind_static_method("WeaverShaderReflection", D_METHOD("reflect", "file", "version"), &WeaverShaderReflection::reflect, DEFVAL(StringName()));

	ClassDB::bind_method(D_METHOD("get_shader_name"), &WeaverShaderReflection::get_shader_name);
	ClassDB::bind_method(D_METHOD("get_stages"), &WeaverShaderReflection::get_stages);
	ClassDB::bind_method(D_METHOD("is_compute"), &WeaverShaderReflection::is_compute);
	ClassDB::bind_method(D_METHOD("get_bindings"), &WeaverShaderReflection::get_bindings);
	ClassDB::bind_method(D_METHOD("get_push_constant_size"), &WeaverShaderReflection::get_push_constant_size);
	ClassDB::bind_method(D_METHOD("get_push_constant_members"), &WeaverShaderReflection::get_push_constant_members);
	ClassDB::bind_method(D_METHOD("get_specialization_constants"), &WeaverShaderReflection::get_specialization_constants);
	ClassDB::bind_method(D_METHOD("get_local_size"), &WeaverShaderReflection::get_local_size);
	ClassDB::bind_method(D_METHOD("get_color_output_count"), &WeaverShaderReflection::get_color_output_count);
	ClassDB::bind_method(D_METHOD("get_layout_hash"), &WeaverShaderReflection::get_layout_hash);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "shader_name"), "", "get_shader_name");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "stages"), "", "get_stages");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "compute"), "", "is_compute");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "bindings", PROPERTY_HINT_ARRAY_TYPE, "Dictionary"), "", "get_bindings");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "push_constant_size"), "", "get_push_constant_size");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "push_constant_members", PROPERTY_HINT_ARRAY_TYPE, "Dictionary"), "", "get_push_constant_members");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "specialization_constants", PROPERTY_HINT_ARRAY_TYPE, "Dictionary"), "", "get_specialization_constants");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3I, "local_size"), "", "get_local_size");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "color_output_count"), "", "get_color_output_count");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "layout_hash"), "", "get_layout_hash");

	BIND_ENUM_CONSTANT(SCALAR_FLOAT);
	BIND_ENUM_CONSTANT(SCALAR_INT);
	BIND_ENUM_CONSTANT(SCALAR_UINT);
	BIND_ENUM_CONSTANT(SCALAR_BOOL);
	BIND_ENUM_CONSTANT(SCALAR_UNSUPPORTED);
}
