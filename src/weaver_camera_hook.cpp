#include "weaver_camera_hook.h"



void WeaverCameraHook::_notification(int p_what)
{
	switch (p_what)
	{
		case Node::NOTIFICATION_ENTER_TREE: {
			print_line("ENTER TRIGGERED");
		} break;
		case Node::NOTIFICATION_READY: {
			print_line("READY TRIGGERED");
		} break;
		case Node::NOTIFICATION_EXIT_TREE: {
			print_line("EXIT TRIGGERED");
		} break;

		// We can use NOTIFICATION_PARENTED to identify changes to the tree and issue warnings !
	}
}

void WeaverCameraHook::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_web_compositor", "web_compositor"), &WeaverCameraHook::set_web_compositor);
	ClassDB::bind_method(D_METHOD("get_web_compositor"), &WeaverCameraHook::get_web_compositor);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "web_compositor", PROPERTY_HINT_RESOURCE_TYPE, "WeaverWebCompositor"), "set_web_compositor", "get_web_compositor");
}

void WeaverCameraHook::set_web_compositor(const Ref<WeaverWebCompositor> &p_web_compositor)
{
	if (web_compositor == p_web_compositor) { return; }
	//Do some setup with the new WeaverWebCompositor...
	web_compositor = p_web_compositor;
}

Ref<WeaverWebCompositor> WeaverCameraHook::get_web_compositor() const
{
	return web_compositor;
}
