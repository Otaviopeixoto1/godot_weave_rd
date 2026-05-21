#include "frame_weaver.h"



void FrameWeaver::_notification(int p_what)
{
	switch (p_what)
	{
		case Node::NOTIFICATION_ENTER_TREE: {

		} break;
		case Node::NOTIFICATION_EXIT_TREE: {

		} break;
	}
}

void FrameWeaver::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_web", "web"), &FrameWeaver::set_web);
	ClassDB::bind_method(D_METHOD("get_web"), &FrameWeaver::get_web);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "web", PROPERTY_HINT_RESOURCE_TYPE, "WeaverWeb"), "set_web", "get_web");
}

void FrameWeaver::set_web(const Ref<WeaverWeb> &p_web)
{
	if (web == p_web) { return; }
	//Do some setup with the new web...
	web = p_web;
}

Ref<WeaverWeb> FrameWeaver::get_web() const
{
	return web;
}
