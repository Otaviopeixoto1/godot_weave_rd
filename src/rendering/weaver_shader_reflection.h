#pragma once

#include "core/object/ref_counted.h"
#include "core/variant/typed_array.h"
#include "servers/rendering/rendering_device.h"

class RDShaderFile;

//
// WeaverShaderReflection represents the layout of a compiled shader from its SPIR-V (bindings, push constants, etc ...).
// All stages of a shader file are merged into one layout: a binding used by several stages must be the same in every one and
// the PUSH CONSTANTS must be IDENTICAL. The bindings are listed by name, so the names must be unique.
//
// A layout_hash is generated for the whole file. It hashes all properties that a generated shader class depends on (bindings, push constants and the pipeline type).
//
// IMPORTANT: Nested structs: Currently, nested struct members are flattened into the format <struct_member_name>.<internal_member_name>
// (e.g. a stuct member "light" will have its properties all listed and prefixed by "light.". For instance: "light.color").
//

class WeaverShaderReflection : public RefCounted
{
	GDCLASS(WeaverShaderReflection, RefCounted);

public:
	enum ScalarType { //TODO: Rename to ScalarType
		SCALAR_FLOAT,
		SCALAR_INT,
		SCALAR_UINT,
		SCALAR_BOOL,
		SCALAR_UNSUPPORTED, // for now we dont support: 16/64 bit types, arrays of structs, etc...
	};

	static Ref<WeaverShaderReflection> reflect(const Ref<RDShaderFile> &p_file, const StringName &p_version = StringName());

	String get_shader_name() const { return shader_name; }
	PackedInt32Array get_stages() const { return stages; }
	bool is_compute() const { return compute; }
	TypedArray<Dictionary> get_bindings() const { return bindings; }
	int get_push_constant_size() const { return push_constant_size; }
	TypedArray<Dictionary> get_push_constant_members() const { return push_constant_members; }
	TypedArray<Dictionary> get_specialization_constants() const { return specialization_constants; }
	Vector3i get_local_size() const { return local_size; }
	int get_color_output_count() const { return color_output_count; }
	int64_t get_layout_hash() const { return layout_hash; }

protected:
	static void _bind_methods();

private:
	String shader_name;
	PackedInt32Array stages;
	bool compute = false;
	TypedArray<Dictionary> bindings;
	int push_constant_size = 0;
	TypedArray<Dictionary> push_constant_members;
	TypedArray<Dictionary> specialization_constants;
	Vector3i local_size;
	int color_output_count = 0;
	int64_t layout_hash = 0;
};

VARIANT_ENUM_CAST(WeaverShaderReflection::ScalarType);
