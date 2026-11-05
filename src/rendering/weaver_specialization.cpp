#include "weaver_specialization.h"

#include "core/object/class_db.h"
#include "servers/rendering/rendering_device_binds.h"


void WeaverSpecialization::_set_constant(int p_constant_id, const Variant &p_value)
{
	ERR_FAIL_COND_MSG(p_constant_id < 0, "WeaverSpecialization: constant ids are not negative.");
	const Variant::Type type = p_value.get_type();
	ERR_FAIL_COND_MSG(type != Variant::BOOL && type != Variant::INT && type != Variant::FLOAT,
			vformat("WeaverSpecialization: constant %d can only be a bool, int or float, not %s.", p_constant_id, Variant::get_type_name(type)));

	values[p_constant_id] = p_value;
}

void WeaverSpecialization::clear()
{
	values.clear();
}

TypedArray<RDPipelineSpecializationConstant> WeaverSpecialization::to_constants() const
{
	// Sorted by id, so the same values always give the same array.
	LocalVector<int> ids;
	for (const KeyValue<int, Variant> &E : values)
	{
		ids.push_back(E.key);
	}
	ids.sort();

	TypedArray<RDPipelineSpecializationConstant> constants;
	for (int id : ids)
	{
		Ref<RDPipelineSpecializationConstant> constant;
		constant.instantiate();
		constant->set_constant_id(id);
		constant->set_value(values[id]);
		constants.push_back(constant);
	}
	return constants;
}


void WeaverSpecialization::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("_set_constant", "constant_id", "value"), &WeaverSpecialization::_set_constant);
	ClassDB::bind_method(D_METHOD("clear"), &WeaverSpecialization::clear);
	ClassDB::bind_method(D_METHOD("to_constants"), &WeaverSpecialization::to_constants);
}
