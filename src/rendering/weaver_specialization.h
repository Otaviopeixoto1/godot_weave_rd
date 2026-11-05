#pragma once

#include "core/object/ref_counted.h"
#include "core/templates/hash_map.h"
#include "core/variant/typed_array.h"

class RDPipelineSpecializationConstant;

//
// WeaverSpecialization holds the specialization constant values of one shader. The generated shader classes extend it with
// one typed property per constant, whose setter calls _set_constant() with the constant id baked in.
//
// to_constants() returns the values that were set, for compute_pipeline_create() / render_pipeline_create(). Constants that
// were never set keep the default value written in the shader.
//
class WeaverSpecialization : public RefCounted
{
	GDCLASS(WeaverSpecialization, RefCounted);

public:
	void _set_constant(int p_constant_id, const Variant &p_value);
	void clear();
	TypedArray<RDPipelineSpecializationConstant> to_constants() const;

protected:
	static void _bind_methods();

private:
	HashMap<int, Variant> values;
};
