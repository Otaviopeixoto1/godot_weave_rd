#include "register_types.h"

#include "core/object/class_db.h"
#include "core/config/engine.h"

#include "src/rendering_weaver.h"
#include "src/frame_weaver.h"
#include "src/resources/weaver_web.h"
#include "src/resources/weaver_web_compositor.h"
#include "src/resources/weaver_web_node.h"


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

	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE)
	{
		ClassDB::register_class<WeaverWebNode>();
		ClassDB::register_class<WeaverWebCompositor>();
		ClassDB::register_class<WeaverWeb>();
		ClassDB::register_class<FrameWeaver>();
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
