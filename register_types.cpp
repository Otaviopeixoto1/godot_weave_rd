#include "register_types.h"

#include "core/object/class_db.h"
#include "core/config/engine.h"

#include "rendering_weaver.h"


static RenderingWeaver *_weaver = nullptr;

void initialize_godot_weave_rd_module(ModuleInitializationLevel p_level)
{
	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS)
	{
		_weaver = memnew(RenderingWeaver);
		ClassDB::register_class<RenderingWeaver>();
		Engine::get_singleton()->add_singleton(Engine::Singleton("RenderingWeaver", RenderingWeaver::get_singleton()));
		return;
	}


}

void uninitialize_godot_weave_rd_module(ModuleInitializationLevel p_level)
{
	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS)
	{
		if (_weaver)
		{
			//_weaver->finish();
			memdelete(_weaver);
		}
		return;
	}
}
