#pragma once

#include "core/object/ref_counted.h"
#include "core/templates/hash_set.h"
#include "core/templates/local_vector.h"
#include "servers/rendering/rendering_device.h"

class RDShaderFile;
class RDSamplerState;

//
// WeaverRenderContext owns the GPU objects of one WeaverWebNode copy (one per camera), so the node doesn't have to free
// them by hand: the runtime calls free_resources() on the render thread right after the node cleanup().
//
// It only creates and owns objects. Pipelines, uniform sets and compute/draw lists stay with the user and every helper
// returns a plain RID, so the regular RenderingDevice API keeps working alongside it.
// - Owned RIDs are freed in reverse creation order. Don't free them by hand: use release() instead.
// - Shaders are compiled once per context. Freeing a shader also frees the pipelines and uniform sets made from it.
// - Samplers are created on the context's device and are managed and shared by every context on that device. 
// 
class WeaverRenderContext : public RefCounted
{
	GDCLASS(WeaverRenderContext, RefCounted);

public:
	WeaverRenderContext();
	virtual ~WeaverRenderContext();

	// Defaults to RenderingServer.get_rendering_device(). 
	RenderingDevice *get_device() const;
	// Sets the current RenderingDevice (only be changed while nothing is owned)
	void set_device(RenderingDevice *p_device);

	RID storage_buffer(uint32_t p_size, const PackedByteArray &p_data = PackedByteArray(), BitField<RD::StorageBufferUsage> p_usage = 0, BitField<RD::BufferCreationBits> p_creation_bits = 0);
	RID uniform_buffer(uint32_t p_size, const PackedByteArray &p_data = PackedByteArray());

	RID get_shader_rid(const Ref<RDShaderFile> &p_file, const StringName &p_version = StringName());
	RID get_sampler(const Ref<RDSamplerState> &p_state);

	// Adds a resource created manually
	RID add(const RID &p_rid);
	void release(const RID &p_rid);
	bool owns(const RID &p_rid) const;
	int get_owned_count() const;

	void free_resources();

protected:
	static void _bind_methods();

private:
	struct CachedShader
	{
		Ref<RDShaderFile> file;
		StringName version;
		RID rid;
	};

	RenderingDevice *device = nullptr;
	LocalVector<RID> owned;
	LocalVector<CachedShader> shaders;
	HashSet<RID> samplers;

	void _release_samplers();
};
