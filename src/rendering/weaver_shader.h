#pragma once

#include "core/object/ref_counted.h"
#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_device_binds.h"

#include "weaver_push_constants.h"
#include "weaver_render_context.h"
#include "weaver_shader_reflection.h"

//
// WeaverShader is the base class of the generated shader classes. It is meant to represent the binding state of all shader stages from one 
// .glsl file. The generated class adds one typed property per binding, a Params push constant block and a Specialization block.
// 
// A generated shader class is responsible for calling _setup() for generating all of the reflection data at runtime
// (see godot_weave_rd_plugin: weaver_shader_class_writer.gd).
//
// Pipelines and compute/draw lists are a user responsability. WeaverShader is meant as a helper to set the uniform sets and 
// push constants on the rd and compute/draw list that are given by the user.
//
// IMPORTANT: Uniform sets on the main device come from UniformSetCacheRD. On other devices the object keeps one uniform set per set index 
// and recreates it when its bindings change (old ones are freed, and the last ones go with the shader).
//
class WeaverShader : public RefCounted
{
	GDCLASS(WeaverShader, RefCounted);

public:
	RID get_rid() const { return rid; }
	Ref<WeaverRenderContext> get_context() const { return context; }
	Ref<RDShaderFile> get_file() const { return file; }
	StringName get_version() const { return version; }
	Ref<WeaverShaderReflection> get_reflection() const { return reflection; }

	// Builds and binds the uniform sets of the shader on a compute list (compute shaders) or draw list (graphics shaders).
	// Empty p_sets binds every set the shader uses.
	Error bind_uniforms(RenderingDevice *p_rd, int64_t p_list, const PackedInt32Array &p_sets = PackedInt32Array());

	// Sends the push constant block, exactly the reflected size.
	Error set_push_constants(RenderingDevice *p_rd, int64_t p_list);

	// Work groups needed to cover p_threads invocations, from the shader's local size (compute only).
	Vector3i groups_for(const Vector3i &p_threads) const;

	// Called by the generated create(). Reflects the shader, checks p_layout_hash against it and compiles it on the context's device.
	// Returns false (with an error) when the class is out of date or the shader doesn't compile.
	bool _setup(const Ref<WeaverRenderContext> &p_context, const Ref<RDShaderFile> &p_file, const StringName &p_version, int64_t p_layout_hash, const Ref<WeaverPushConstants> &p_push_constants);

	// Called by the generated binding properties. An invalid RID clears the binding.
	void _bind(int p_set, int p_binding, RD::UniformType p_type, const RID &p_rid);
	void _bind_pair(int p_set, int p_binding, RD::UniformType p_type, const RID &p_sampler, const RID &p_texture);
	void _bind_array(int p_set, int p_binding, RD::UniformType p_type, const Array &p_rids);

	// One sampler per texture, or a single sampler used for all of them.
	void _bind_pair_array(int p_set, int p_binding, RD::UniformType p_type, const Array &p_samplers, const Array &p_textures);

protected:
	static void _bind_methods();

private:
	struct ExpectedBinding
	{
		uint32_t set = 0;
		uint32_t binding = 0;
		RD::UniformType type = RD::UNIFORM_TYPE_MAX;
		uint32_t count = 1;
		String name;
	};
	struct ExpectedOrder
	{
		bool operator()(const ExpectedBinding &p_a, const ExpectedBinding &p_b) const
		{
			return p_a.set != p_b.set ? p_a.set < p_b.set : p_a.binding < p_b.binding;
		}
	};
	struct BoundUniform
	{
		RD::UniformType type = RD::UNIFORM_TYPE_MAX; // Can be the _DYNAMIC variant of the reflected buffer type.
		Vector<RID> ids; // In RD::Uniform order (sampler, texture, sampler, texture... for combined samplers).
	};
	struct LocalUniformSet
	{
		LocalVector<RD::Uniform> uniforms;
		RID rid;
	};
	struct PushConstantMember
	{
		int offset = 0;
		String name;
	};

	StringName version;
	Ref<WeaverRenderContext> context;
	Ref<RDShaderFile> file;
	Ref<WeaverShaderReflection> reflection;
	Ref<WeaverPushConstants> push_constants;
	RID rid;
	String path; //Rename to file_path

	LocalVector<ExpectedBinding> expected; // Sorted by set, then binding.
	PackedInt32Array used_sets;
	HashMap<uint32_t, LocalUniformSet> local_sets;

	// Binding maps
	HashMap<uint64_t, uint32_t> expected_index; // _key(set, binding) -> index in "expected" vector.
	HashMap<uint64_t, BoundUniform> bound; // _key(set, binding) -> BoundUniform.

	// Appends the bits of a set and a binding ids into a single key
	static uint64_t _key(uint32_t p_set, uint32_t p_binding) { return (uint64_t(p_set) << 32) | p_binding; }

	bool _check_ready(const char *p_function) const;
	bool _check_device(RenderingDevice *p_rd) const;
	bool _check_binding(int p_set, int p_binding, RD::UniformType p_type) const;
	void _store(int p_set, int p_binding, RD::UniformType p_type, const Vector<RID> &p_ids);
	RID _get_uniform_set(RenderingDevice *p_rd, uint32_t p_set, const LocalVector<RD::Uniform> &p_uniforms);
};
