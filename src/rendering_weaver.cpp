#include "rendering_weaver.h"
#include "core/object/class_db.h"



RenderingWeaver *RenderingWeaver::singleton = nullptr;

RenderingWeaver *RenderingWeaver::get_singleton()
{
	return singleton;
}

Ref<GDScript> RenderingWeaver::get_weaver_node_script()
{
	if (!weaver_node_script.is_valid())
	{
		weaver_node_script.instantiate();
		weaver_node_script->set_source_code(
			"@tool\n"
			"extends WeaverWebNode\n"
			"func _render_callback(p_effect_callback_type, p_render_data):\n"
			"   _internal_weaver_render_callback(p_effect_callback_type, p_render_data)\n"
		);
		Error err = weaver_node_script->reload();
		ERR_FAIL_COND_V_MSG(err != OK, nullptr, "FAILED COMPILING SCRIPT");
	}

	return weaver_node_script;
}

void RenderingWeaver::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("add", "value"), &RenderingWeaver::add);
	//This works because its a simple method that we are binding. It might be necessary to use a dummy class with _bind_methods() defined just to bind the correct singleton methods
	//-> This problem is created because ClassDB::bind_method might not use the singleton instance for binding the function pointer but in a dummy class
	//   we can call the static get_singleton(), which solver it (?)
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

