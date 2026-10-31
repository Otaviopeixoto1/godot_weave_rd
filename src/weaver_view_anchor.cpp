#include "weaver_view_anchor.h"
#include "rendering_weaver.h"

#include "core/object/class_db.h"
#include "core/object/callable_mp.h"
#include "scene/main/viewport.h"


void WeaverViewAnchor::_notification(int p_what)
{
	RenderingWeaver *weaver = RenderingWeaver::get_singleton();
	if (weaver == nullptr) return;

	switch (p_what) {
		case Node::NOTIFICATION_ENTER_TREE: {
			weaver->connect(SNAME("bindings_changed"), callable_mp((Node *)this, &Node::update_configuration_warnings));
			_register();
		} break;
		case Node::NOTIFICATION_EXIT_TREE: {
			weaver->disconnect(SNAME("bindings_changed"), callable_mp((Node *)this, &Node::update_configuration_warnings));
			weaver->unregister_view_anchor(this);
		} break;
	}
}

void WeaverViewAnchor::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_slot", "slot"), &WeaverViewAnchor::set_slot);
	ClassDB::bind_method(D_METHOD("get_slot"), &WeaverViewAnchor::get_slot);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "slot", PROPERTY_HINT_RESOURCE_TYPE, "WeaverViewSlot"), "set_slot", "get_slot");
}

void WeaverViewAnchor::set_slot(const Ref<WeaverViewSlot> &p_slot)
{
	if (slot == p_slot) { return; }
	slot = p_slot;

	if (is_inside_tree())
	{
		_register();
	}
}

Ref<WeaverViewSlot> WeaverViewAnchor::get_slot() const
{
	return slot;
}

void WeaverViewAnchor::_register()
{
	RenderingWeaver::get_singleton()->register_view_anchor(this, get_viewport(), slot);
}

PackedStringArray WeaverViewAnchor::get_configuration_warnings() const
{
	PackedStringArray warnings = Node::get_configuration_warnings();

	const RenderingWeaver *weaver = RenderingWeaver::get_singleton();
	if (weaver != nullptr)
	{
		warnings.append_array(weaver->get_warnings(this));
	}

	return warnings;
}
