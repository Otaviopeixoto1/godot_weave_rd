#include "weaver_push_constants.h"

#include "core/math/projection.h"
#include "core/math/transform_3d.h"
#include "core/object/class_db.h"


void WeaverPushConstants::set_block_size(int p_size)
{
	ERR_FAIL_COND(p_size < 0);

	if (data.size() > p_size)
	{
		ERR_PRINT(vformat("WeaverPushConstants: values were written up to byte %d, past the end of the %d byte push constant block.", data.size(), p_size));
	}

	data.resize_initialized(p_size);
	size_fixed = true;
}

uint8_t *WeaverPushConstants::_reserve(int p_offset, int p_bytes)
{
	ERR_FAIL_COND_V(p_offset < 0, nullptr);

	const int end = p_offset + p_bytes;
	if (end > data.size())
	{
		ERR_FAIL_COND_V_MSG(size_fixed, nullptr,
				vformat("WeaverPushConstants: writing %d bytes at offset %d goes past the end of the %d byte push constant block.", p_bytes, p_offset, data.size()));
		data.resize_initialized(end);
	}

	return data.ptrw() + p_offset;
}

void WeaverPushConstants::_write_floats(int p_offset, const float *p_values, int p_count)
{
	uint8_t *dst = _reserve(p_offset, p_count * 4);
	if (dst == nullptr) return;

	memcpy(dst, p_values, p_count * 4);
}

void WeaverPushConstants::_write_u32s(int p_offset, const uint32_t *p_values, int p_count)
{
	uint8_t *dst = _reserve(p_offset, p_count * 4);
	if (dst == nullptr) return;

	memcpy(dst, p_values, p_count * 4);
}

void WeaverPushConstants::_write_matrix(int p_offset, const float *p_values, int p_columns, int p_rows, int p_stride, bool p_row_major)
{
	// Column major: each column is a vector of p_rows floats, p_stride bytes apart. Row major: the transpose.
	const int vectors = p_row_major ? p_rows : p_columns;
	const int vector_size = p_row_major ? p_columns : p_rows;
	ERR_FAIL_COND_MSG(p_stride < vector_size * 4, vformat("WeaverPushConstants: matrix stride %d is smaller than its %d byte vectors.", p_stride, vector_size * 4));

	uint8_t *dst = _reserve(p_offset, (vectors - 1) * p_stride + vector_size * 4);
	if (dst == nullptr) return;

	for (int v = 0; v < vectors; v++)
	{
		for (int e = 0; e < vector_size; e++)
		{
			const int column = p_row_major ? e : v;
			const int row = p_row_major ? v : e;
			memcpy(dst + v * p_stride + e * 4, &p_values[column * p_rows + row], 4);
		}
	}
}

bool WeaverPushConstants::_check_array(int p_size, int p_stride, int p_count, int p_element_bytes) const
{
	ERR_FAIL_COND_V_MSG(p_size > p_count, false, vformat("WeaverPushConstants: %d values written to an array of %d.", p_size, p_count));
	ERR_FAIL_COND_V_MSG(p_stride < p_element_bytes, false, vformat("WeaverPushConstants: array stride %d is smaller than its %d byte elements.", p_stride, p_element_bytes));
	return true;
}


//
// Scalars and vectors
//
void WeaverPushConstants::_write_float(int p_offset, float p_value)
{
	_write_floats(p_offset, &p_value, 1);
}

void WeaverPushConstants::_write_int(int p_offset, int64_t p_value)
{
	ERR_FAIL_COND_MSG(p_value < INT32_MIN || p_value > INT32_MAX, vformat("WeaverPushConstants: %d does not fit in a 32 bit int.", p_value));

	const int32_t value = int32_t(p_value);
	_write_u32s(p_offset, reinterpret_cast<const uint32_t *>(&value), 1);
}

void WeaverPushConstants::_write_uint(int p_offset, int64_t p_value)
{
	ERR_FAIL_COND_MSG(p_value < 0, vformat("WeaverPushConstants: %d is negative and can't be written to a uint.", p_value));
	ERR_FAIL_COND_MSG(p_value > UINT32_MAX, vformat("WeaverPushConstants: %d does not fit in a 32 bit uint.", p_value));

	const uint32_t value = uint32_t(p_value);
	_write_u32s(p_offset, &value, 1);
}

void WeaverPushConstants::_write_bool(int p_offset, bool p_value)
{
	// GLSL bools in a block are 4 bytes.
	const uint32_t value = p_value ? 1 : 0;
	_write_u32s(p_offset, &value, 1);
}

void WeaverPushConstants::_write_vec2(int p_offset, const Vector2 &p_value)
{
	const float values[2] = { float(p_value.x), float(p_value.y) };
	_write_floats(p_offset, values, 2);
}

void WeaverPushConstants::_write_vec3(int p_offset, const Vector3 &p_value)
{
	const float values[3] = { float(p_value.x), float(p_value.y), float(p_value.z) };
	_write_floats(p_offset, values, 3);
}

void WeaverPushConstants::_write_vec4(int p_offset, const Vector4 &p_value)
{
	const float values[4] = { float(p_value.x), float(p_value.y), float(p_value.z), float(p_value.w) };
	_write_floats(p_offset, values, 4);
}

void WeaverPushConstants::_write_ivec2(int p_offset, const Vector2i &p_value)
{
	const int32_t values[2] = { p_value.x, p_value.y };
	_write_u32s(p_offset, reinterpret_cast<const uint32_t *>(values), 2);
}

void WeaverPushConstants::_write_ivec3(int p_offset, const Vector3i &p_value)
{
	const int32_t values[3] = { p_value.x, p_value.y, p_value.z };
	_write_u32s(p_offset, reinterpret_cast<const uint32_t *>(values), 3);
}

void WeaverPushConstants::_write_ivec4(int p_offset, const Vector4i &p_value)
{
	const int32_t values[4] = { p_value.x, p_value.y, p_value.z, p_value.w };
	_write_u32s(p_offset, reinterpret_cast<const uint32_t *>(values), 4);
}

void WeaverPushConstants::_write_uvec2(int p_offset, const Vector2i &p_value)
{
	ERR_FAIL_COND_MSG(p_value.x < 0 || p_value.y < 0, vformat("WeaverPushConstants: %s has a negative component and can't be written to a uvec2.", p_value));
	_write_ivec2(p_offset, p_value);
}

void WeaverPushConstants::_write_uvec3(int p_offset, const Vector3i &p_value)
{
	ERR_FAIL_COND_MSG(p_value.x < 0 || p_value.y < 0 || p_value.z < 0, vformat("WeaverPushConstants: %s has a negative component and can't be written to a uvec3.", p_value));
	_write_ivec3(p_offset, p_value);
}

void WeaverPushConstants::_write_uvec4(int p_offset, const Vector4i &p_value)
{
	ERR_FAIL_COND_MSG(p_value.x < 0 || p_value.y < 0 || p_value.z < 0 || p_value.w < 0, vformat("WeaverPushConstants: %s has a negative component and can't be written to a uvec4.", p_value));
	_write_ivec4(p_offset, p_value);
}


//
// Matrices
//
void WeaverPushConstants::_write_mat3(int p_offset, const Basis &p_value, int p_stride, bool p_row_major)
{
	// GLSL column i is the Basis column i (the axis), like Godot's own mat3 uniforms.
	float values[9];
	for (int c = 0; c < 3; c++)
	{
		const Vector3 column = p_value.get_column(c);
		values[c * 3 + 0] = column.x;
		values[c * 3 + 1] = column.y;
		values[c * 3 + 2] = column.z;
	}
	_write_matrix(p_offset, values, 3, 3, p_stride, p_row_major);
}

void WeaverPushConstants::_write_mat4(int p_offset, const Projection &p_value, int p_stride, bool p_row_major)
{
	float values[16];
	for (int c = 0; c < 4; c++)
	{
		for (int r = 0; r < 4; r++)
		{
			values[c * 4 + r] = p_value.columns[c][r];
		}
	}
	_write_matrix(p_offset, values, 4, 4, p_stride, p_row_major);
}

void WeaverPushConstants::_write_mat4_transform(int p_offset, const Transform3D &p_value, int p_stride, bool p_row_major)
{
	_write_mat4(p_offset, Projection(p_value), p_stride, p_row_major);
}


//
// Arrays
//
void WeaverPushConstants::_write_float_array(int p_offset, const PackedFloat32Array &p_values, int p_stride, int p_count)
{
	if (!_check_array(p_values.size(), p_stride, p_count, 4) || p_values.is_empty()) return;

	uint8_t *dst = _reserve(p_offset, (p_values.size() - 1) * p_stride + 4);
	if (dst == nullptr) return;

	for (int i = 0; i < p_values.size(); i++)
	{
		memcpy(dst + i * p_stride, &p_values[i], 4);
	}
}

void WeaverPushConstants::_write_int_array(int p_offset, const PackedInt32Array &p_values, int p_stride, int p_count)
{
	if (!_check_array(p_values.size(), p_stride, p_count, 4) || p_values.is_empty()) return;

	uint8_t *dst = _reserve(p_offset, (p_values.size() - 1) * p_stride + 4);
	if (dst == nullptr) return;

	for (int i = 0; i < p_values.size(); i++)
	{
		memcpy(dst + i * p_stride, &p_values[i], 4);
	}
}

void WeaverPushConstants::_write_uint_array(int p_offset, const PackedInt32Array &p_values, int p_stride, int p_count)
{
	for (int i = 0; i < p_values.size(); i++)
	{
		ERR_FAIL_COND_MSG(p_values[i] < 0, vformat("WeaverPushConstants: element %d is negative (%d) and can't be written to a uint array.", i, p_values[i]));
	}
	_write_int_array(p_offset, p_values, p_stride, p_count);
}

void WeaverPushConstants::_write_vec2_array(int p_offset, const PackedVector2Array &p_values, int p_stride, int p_count)
{
	if (!_check_array(p_values.size(), p_stride, p_count, 8) || p_values.is_empty()) return;

	uint8_t *dst = _reserve(p_offset, (p_values.size() - 1) * p_stride + 8);
	if (dst == nullptr) return;

	for (int i = 0; i < p_values.size(); i++)
	{
		const float values[2] = { float(p_values[i].x), float(p_values[i].y) };
		memcpy(dst + i * p_stride, values, 8);
	}
}

void WeaverPushConstants::_write_vec3_array(int p_offset, const PackedVector3Array &p_values, int p_stride, int p_count)
{
	if (!_check_array(p_values.size(), p_stride, p_count, 12) || p_values.is_empty()) return;

	uint8_t *dst = _reserve(p_offset, (p_values.size() - 1) * p_stride + 12);
	if (dst == nullptr) return;

	for (int i = 0; i < p_values.size(); i++)
	{
		const float values[3] = { float(p_values[i].x), float(p_values[i].y), float(p_values[i].z) };
		memcpy(dst + i * p_stride, values, 12);
	}
}

void WeaverPushConstants::_write_vec4_array(int p_offset, const PackedVector4Array &p_values, int p_stride, int p_count)
{
	if (!_check_array(p_values.size(), p_stride, p_count, 16) || p_values.is_empty()) return;

	uint8_t *dst = _reserve(p_offset, (p_values.size() - 1) * p_stride + 16);
	if (dst == nullptr) return;

	for (int i = 0; i < p_values.size(); i++)
	{
		const float values[4] = { float(p_values[i].x), float(p_values[i].y), float(p_values[i].z), float(p_values[i].w) };
		memcpy(dst + i * p_stride, values, 16);
	}
}


void WeaverPushConstants::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_data"), &WeaverPushConstants::get_data);
	ClassDB::bind_method(D_METHOD("get_size"), &WeaverPushConstants::get_size);

	ClassDB::bind_method(D_METHOD("_write_float", "offset", "value"), &WeaverPushConstants::_write_float);
	ClassDB::bind_method(D_METHOD("_write_int", "offset", "value"), &WeaverPushConstants::_write_int);
	ClassDB::bind_method(D_METHOD("_write_uint", "offset", "value"), &WeaverPushConstants::_write_uint);
	ClassDB::bind_method(D_METHOD("_write_bool", "offset", "value"), &WeaverPushConstants::_write_bool);

	ClassDB::bind_method(D_METHOD("_write_vec2", "offset", "value"), &WeaverPushConstants::_write_vec2);
	ClassDB::bind_method(D_METHOD("_write_vec3", "offset", "value"), &WeaverPushConstants::_write_vec3);
	ClassDB::bind_method(D_METHOD("_write_vec4", "offset", "value"), &WeaverPushConstants::_write_vec4);
	ClassDB::bind_method(D_METHOD("_write_ivec2", "offset", "value"), &WeaverPushConstants::_write_ivec2);
	ClassDB::bind_method(D_METHOD("_write_ivec3", "offset", "value"), &WeaverPushConstants::_write_ivec3);
	ClassDB::bind_method(D_METHOD("_write_ivec4", "offset", "value"), &WeaverPushConstants::_write_ivec4);
	ClassDB::bind_method(D_METHOD("_write_uvec2", "offset", "value"), &WeaverPushConstants::_write_uvec2);
	ClassDB::bind_method(D_METHOD("_write_uvec3", "offset", "value"), &WeaverPushConstants::_write_uvec3);
	ClassDB::bind_method(D_METHOD("_write_uvec4", "offset", "value"), &WeaverPushConstants::_write_uvec4);

	ClassDB::bind_method(D_METHOD("_write_mat3", "offset", "value", "stride", "row_major"), &WeaverPushConstants::_write_mat3, DEFVAL(16), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("_write_mat4", "offset", "value", "stride", "row_major"), &WeaverPushConstants::_write_mat4, DEFVAL(16), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("_write_mat4_transform", "offset", "value", "stride", "row_major"), &WeaverPushConstants::_write_mat4_transform, DEFVAL(16), DEFVAL(false));

	ClassDB::bind_method(D_METHOD("_write_float_array", "offset", "values", "stride", "count"), &WeaverPushConstants::_write_float_array);
	ClassDB::bind_method(D_METHOD("_write_int_array", "offset", "values", "stride", "count"), &WeaverPushConstants::_write_int_array);
	ClassDB::bind_method(D_METHOD("_write_uint_array", "offset", "values", "stride", "count"), &WeaverPushConstants::_write_uint_array);
	ClassDB::bind_method(D_METHOD("_write_vec2_array", "offset", "values", "stride", "count"), &WeaverPushConstants::_write_vec2_array);
	ClassDB::bind_method(D_METHOD("_write_vec3_array", "offset", "values", "stride", "count"), &WeaverPushConstants::_write_vec3_array);
	ClassDB::bind_method(D_METHOD("_write_vec4_array", "offset", "values", "stride", "count"), &WeaverPushConstants::_write_vec4_array);
}
