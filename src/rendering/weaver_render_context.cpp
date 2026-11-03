#include "weaver_render_context.h"
#include "rendering_weaver.h"

#include "core/object/class_db.h"
#include "servers/rendering/rendering_device_binds.h"
#include "servers/rendering/rendering_server.h"


WeaverRenderContext::WeaverRenderContext()
{
	if (RenderingServer *rs = RenderingServer::get_singleton())
	{
		device = rs->get_rendering_device();
	}
}

WeaverRenderContext::~WeaverRenderContext()
{
	// Not freed here: this may run on any thread, and after the device is gone.
	if (get_owned_count() > 0)
	{
		WARN_PRINT(vformat("WeaverRenderContext freed while still owning %d RIDs. Call free_resources() first.", get_owned_count()));
	}
}

RenderingDevice *WeaverRenderContext::get_device() const
{
	return device;
}

void WeaverRenderContext::set_device(RenderingDevice *p_device)
{
	ERR_FAIL_COND_MSG(get_owned_count() > 0, "The device of a WeaverRenderContext can't change while it owns RIDs. Call free_resources() first.");
	device = p_device;
}

//
// Owned objects
//
RID WeaverRenderContext::storage_buffer(uint32_t p_size, const PackedByteArray &p_data, BitField<RD::StorageBufferUsage> p_usage, BitField<RD::BufferCreationBits> p_creation_bits)
{
	RenderingDevice *rd = get_device();
	ERR_FAIL_NULL_V_MSG(rd, RID(), "WeaverRenderContext: RenderingDevice is null");

	return add(rd->storage_buffer_create(p_size, p_data, p_usage, p_creation_bits));
}

RID WeaverRenderContext::uniform_buffer(uint32_t p_size, const PackedByteArray &p_data)
{
	RenderingDevice *rd = get_device();
	ERR_FAIL_NULL_V_MSG(rd, RID(), "WeaverRenderContext: RenderingDevice is null");

	return add(rd->uniform_buffer_create(p_size, p_data));
}

RID WeaverRenderContext::add(const RID &p_rid)
{
	ERR_FAIL_COND_V_MSG(samplers.has(p_rid), p_rid, "Samplers from get_sampler() are shared and already held by the context.");
	if (p_rid.is_valid() && !owned.has(p_rid))
	{
		owned.push_back(p_rid);
	}
	return p_rid;
}

void WeaverRenderContext::release(const RID &p_rid)
{
	if (samplers.has(p_rid))
	{
		// Shared: only drops this context's reference.
		samplers.erase(p_rid);
		RenderingWeaver::get_singleton()->release_sampler(p_rid);
		return;
	}

	ERR_FAIL_COND_MSG(!owned.has(p_rid), "This RID is not owned by the WeaverRenderContext.");
	owned.erase(p_rid);
	for (uint32_t i = 0; i < shaders.size(); i++)
	{
		if (shaders[i].rid == p_rid)
		{
			shaders.remove_at(i);
			break;
		}
	}
	if (device != nullptr)
	{
		device->free_rid(p_rid);
	}
}

bool WeaverRenderContext::owns(const RID &p_rid) const
{
	return owned.has(p_rid) || samplers.has(p_rid);
}

int WeaverRenderContext::get_owned_count() const
{
	return owned.size() + samplers.size();
}

void WeaverRenderContext::free_resources()
{
	// Reverse creation order: objects made from others (pipelines, uniform sets) go before what they depend on.
	if (device != nullptr)
	{
		for (int64_t i = int64_t(owned.size()) - 1; i >= 0; i--)
		{
			device->free_rid(owned[i]);
		}
	}
	owned.clear();
	shaders.clear();

	// After the owned RIDs: uniform sets made by the context may use these samplers.
	_release_samplers();
}

void WeaverRenderContext::_release_samplers()
{
	RenderingWeaver *weaver = RenderingWeaver::get_singleton();
	if (weaver != nullptr)
	{
		for (const RID &rid : samplers)
		{
			weaver->release_sampler(rid);
		}
	}
	samplers.clear();
}


//
// Shaders
//
RID WeaverRenderContext::get_shader_rid(const Ref<RDShaderFile> &p_file, const StringName &p_version)
{
	ERR_FAIL_COND_V(p_file.is_null(), RID());
	for (const CachedShader &cached : shaders)
	{
		if (cached.file == p_file && cached.version == p_version) return cached.rid;
	}

	RenderingDevice *rd = get_device();
	ERR_FAIL_NULL_V_MSG(rd, RID(), "WeaverRenderContext: RenderingDevice is null");

	const String name = p_file->get_path().is_empty() ? String("<unsaved shader>") : p_file->get_path();
	ERR_FAIL_COND_V_MSG(!p_file->get_base_error().is_empty(), RID(), vformat("%s has errors and can't be compiled.", name));
	ERR_FAIL_COND_V_MSG(!p_file->get_version_list().has(p_version), RID(), vformat("%s has no version \"%s\".", name, p_version));

	const Ref<RDShaderSPIRV> spirv = p_file->get_spirv(p_version);
	for (int s = 0; s < RD::SHADER_STAGE_MAX; s++)
	{
		ERR_FAIL_COND_V_MSG(!spirv->get_stage_compile_error(RD::ShaderStage(s)).is_empty(), RID(),
				vformat("%s: the %s stage failed to compile.", name, RD::SHADER_STAGE_NAMES[s]));
	}

	const RID rid = rd->shader_create_from_spirv(p_file->get_spirv_stages(p_version), name);
	ERR_FAIL_COND_V_MSG(!rid.is_valid(), RID(), vformat("%s could not be created on the RenderingDevice.", name));

	add(rid);
	shaders.push_back({ p_file, p_version, rid });
	return rid;
}


//
// Samplers
//
RID WeaverRenderContext::get_sampler(const Ref<RDSamplerState> &p_state)
{
	ERR_FAIL_COND_V(p_state.is_null(), RID());

	RenderingDevice *rd = get_device();
	ERR_FAIL_NULL_V_MSG(rd, RID(), "WeaverRenderContext: RenderingDevice is null");

	RenderingWeaver *weaver = RenderingWeaver::get_singleton();
	ERR_FAIL_NULL_V(weaver, RID());

	RD::SamplerState state;
	state.mag_filter = p_state->get_mag_filter();
	state.min_filter = p_state->get_min_filter();
	state.mip_filter = p_state->get_mip_filter();
	state.repeat_u = p_state->get_repeat_u();
	state.repeat_v = p_state->get_repeat_v();
	state.repeat_w = p_state->get_repeat_w();
	state.lod_bias = p_state->get_lod_bias();
	state.use_anisotropy = p_state->get_use_anisotropy();
	state.anisotropy_max = p_state->get_anisotropy_max();
	state.enable_compare = p_state->get_enable_compare();
	state.compare_op = p_state->get_compare_op();
	state.min_lod = p_state->get_min_lod();
	state.max_lod = p_state->get_max_lod();
	state.border_color = p_state->get_border_color();
	state.unnormalized_uvw = p_state->get_unnormalized_uvw();

	const RID rid = weaver->acquire_sampler(rd, state);
	if (!rid.is_valid()) return RID();

	if (samplers.has(rid))
	{
		// Already held by this context: drop the reference acquire_sampler() just added:
		weaver->release_sampler(rid);
	}
	else
	{
		samplers.insert(rid);
	}
	return rid;
}


void WeaverRenderContext::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_device"), &WeaverRenderContext::get_device);
	ClassDB::bind_method(D_METHOD("set_device", "device"), &WeaverRenderContext::set_device);

	ClassDB::bind_method(D_METHOD("storage_buffer", "size", "data", "usage", "creation_bits"), &WeaverRenderContext::storage_buffer, DEFVAL(PackedByteArray()), DEFVAL(0), DEFVAL(0));
	ClassDB::bind_method(D_METHOD("uniform_buffer", "size", "data"), &WeaverRenderContext::uniform_buffer, DEFVAL(PackedByteArray()));
	ClassDB::bind_method(D_METHOD("add", "rid"), &WeaverRenderContext::add);
	ClassDB::bind_method(D_METHOD("release", "rid"), &WeaverRenderContext::release);
	ClassDB::bind_method(D_METHOD("owns", "rid"), &WeaverRenderContext::owns);
	ClassDB::bind_method(D_METHOD("get_owned_count"), &WeaverRenderContext::get_owned_count);

	ClassDB::bind_method(D_METHOD("get_shader_rid", "file", "version"), &WeaverRenderContext::get_shader_rid, DEFVAL(StringName()));

	ClassDB::bind_method(D_METHOD("get_sampler", "state"), &WeaverRenderContext::get_sampler);

	ClassDB::bind_method(D_METHOD("free_resources"), &WeaverRenderContext::free_resources);
}
