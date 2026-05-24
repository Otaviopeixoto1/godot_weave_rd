#include "frame_weaver.h"



void FrameWeaver::_notification(int p_what)
{
	//todo: register the web into the RenderingWeaver server
}

void FrameWeaver::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_web", "web"), &FrameWeaver::set_web);
	ClassDB::bind_method(D_METHOD("get_web"), &FrameWeaver::get_web);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "web", PROPERTY_HINT_RESOURCE_TYPE, "WeaverWeb"), "set_web", "get_web");
}

void FrameWeaver::set_web(const Ref<WeaverWeb> &p_web)
{
	if (web == p_web) { return; }
	//Do some setup with the new WeaverWebCompositor...
	web = p_web;
}

Ref<WeaverWeb> FrameWeaver::get_web() const
{
	return web;
}
