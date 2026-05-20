#include "rendering_weaver.h"
#include "core/object/class_db.h"



RenderingWeaver *RenderingWeaver::singleton = nullptr;

RenderingWeaver *RenderingWeaver::get_singleton()
{
	return singleton;
}

void RenderingWeaver::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("add", "value"), &RenderingWeaver::add);
}

void RenderingWeaver::add(int value)
{
	print_line(value);
}

RenderingWeaver::RenderingWeaver()
{
	ERR_FAIL_COND_MSG(singleton != nullptr, "RenderingWeaver: Singleton already exists");
	singleton = this;
}

