#include "weaver_spider_nest.h"
#include "rendering_weaver.h"

#include "core/object/class_db.h"
#include "core/object/callable_mp.h"
#include "scene/3d/camera_3d.h"


void WeaverSpiderNest::_notification(int p_what)
{
	RenderingWeaver *weaver = RenderingWeaver::get_singleton();
	if (weaver == nullptr) return;

	switch (p_what)
	{
		case Node::NOTIFICATION_ENTER_TREE: {
			weaver->connect(SNAME("bindings_changed"), callable_mp((Node *)this, &Node::update_configuration_warnings));
			_register();
		} break;
		case Node::NOTIFICATION_EXIT_TREE: {
			weaver->disconnect(SNAME("bindings_changed"), callable_mp((Node *)this, &Node::update_configuration_warnings));
			weaver->unregister_camera_nest(this);
		} break;
	}
}

void WeaverSpiderNest::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_slot", "slot"), &WeaverSpiderNest::set_slot);
	ClassDB::bind_method(D_METHOD("get_slot"), &WeaverSpiderNest::get_slot);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "slot", PROPERTY_HINT_RESOURCE_TYPE, "WeaverCameraSlot"), "set_slot", "get_slot");
}

void WeaverSpiderNest::set_slot(const Ref<WeaverCameraSlot> &p_slot)
{
	if (slot == p_slot) { return; }
	slot = p_slot;

	if (is_inside_tree())
	{
		_register();
	}
}

Ref<WeaverCameraSlot> WeaverSpiderNest::get_slot() const {
	return slot;
}

void WeaverSpiderNest::_register()
{
	// Only a nest directly under a Camera3D binds; anything else is reported as a configuration warning.
	Camera3D *camera = Object::cast_to<Camera3D>(get_parent());
	if (camera != nullptr)
	{
		RenderingWeaver::get_singleton()->register_camera_nest(this, camera, slot);
	}
	else
	{
		RenderingWeaver::get_singleton()->unregister_camera_nest(this);
	}
}

PackedStringArray WeaverSpiderNest::get_configuration_warnings() const
{
	PackedStringArray warnings = Node::get_configuration_warnings();

	if (Object::cast_to<Camera3D>(get_parent()) == nullptr)
	{
		warnings.push_back("Parent must be a Camera3D.");
		return warnings;
	}

	const RenderingWeaver *weaver = RenderingWeaver::get_singleton();
	if (weaver != nullptr)
	{
		warnings.append_array(weaver->get_warnings(this));
	}

	return warnings;
}
