#include "weaver_slot.h"
#include "core/object/class_db.h"


void WeaverSlot::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_description", "description"), &WeaverSlot::set_description);
	ClassDB::bind_method(D_METHOD("get_description"), &WeaverSlot::get_description);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "description", PROPERTY_HINT_MULTILINE_TEXT), "set_description", "get_description");

	ClassDB::bind_method(D_METHOD("is_shareable"), &WeaverSlot::is_shareable);
}

void WeaverSlot::set_description(const String &p_description)
{
	description = p_description;
	emit_changed();
}

String WeaverSlot::get_description() const
{
	return description;
}

bool WeaverSlot::is_shareable() const
{
	// is_built_in() covers unsaved resources, sub-resources ("::") and local scene resources.
	return !is_built_in();
}
