#pragma once

#include "core/object/ref_counted.h"
#include "core/templates/hash_set.h"

//
// WeaverPushConstants is the byte buffer of one push constant block using std430 layout. The generated shader classes extend it with one typed
// property per member, whose setter calls a _write_* method with the member that has an offset baked in.
//
class WeaverPushConstants : public RefCounted
{
	GDCLASS(WeaverPushConstants, RefCounted);

public:
	PackedByteArray get_data() const { return data; }
	int get_size() const { return data.size(); }

	// Fixes the buffer to the reflected block size. Bytes already written are kept.
	void set_block_size(int p_size);
	const Vector<uint8_t> &get_data_ref() const { return data; }

	void _write_float(int p_offset, float p_value);
	void _write_int(int p_offset, int64_t p_value);
	void _write_uint(int p_offset, int64_t p_value);
	void _write_bool(int p_offset, bool p_value);

	void _write_vec2(int p_offset, const Vector2 &p_value);
	void _write_vec3(int p_offset, const Vector3 &p_value);
	void _write_vec4(int p_offset, const Vector4 &p_value);
	void _write_ivec2(int p_offset, const Vector2i &p_value);
	void _write_ivec3(int p_offset, const Vector3i &p_value);
	void _write_ivec4(int p_offset, const Vector4i &p_value);
	void _write_uvec2(int p_offset, const Vector2i &p_value);
	void _write_uvec3(int p_offset, const Vector3i &p_value);
	void _write_uvec4(int p_offset, const Vector4i &p_value);

	void _write_mat3(int p_offset, const Basis &p_value, int p_stride = 16, bool p_row_major = false);
	void _write_mat4(int p_offset, const Projection &p_value, int p_stride = 16, bool p_row_major = false);
	void _write_mat4_transform(int p_offset, const Transform3D &p_value, int p_stride = 16, bool p_row_major = false);

	// p_count is the array length in the shader: more values than that is an error, fewer leaves the rest untouched.
	void _write_float_array(int p_offset, const PackedFloat32Array &p_values, int p_stride, int p_count);
	void _write_int_array(int p_offset, const PackedInt32Array &p_values, int p_stride, int p_count);
	void _write_uint_array(int p_offset, const PackedInt32Array &p_values, int p_stride, int p_count);
	void _write_vec2_array(int p_offset, const PackedVector2Array &p_values, int p_stride, int p_count);
	void _write_vec3_array(int p_offset, const PackedVector3Array &p_values, int p_stride, int p_count);
	void _write_vec4_array(int p_offset, const PackedVector4Array &p_values, int p_stride, int p_count);

protected:
	static void _bind_methods();

private:
	Vector<uint8_t> data;
	bool size_fixed = false;

	// Returns where p_bytes can be written at p_offset (growing the buffer while the size isn't fixed), or nullptr.
	uint8_t *_reserve(int p_offset, int p_bytes);
	void _write_floats(int p_offset, const float *p_values, int p_count);
	void _write_u32s(int p_offset, const uint32_t *p_values, int p_count);

	// p_columns column vectors of p_rows floats each (column major order in p_values).
	void _write_matrix(int p_offset, const float *p_values, int p_columns, int p_rows, int p_stride, bool p_row_major);
	bool _check_array(int p_size, int p_stride, int p_count, int p_element_bytes) const;
};
