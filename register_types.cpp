#include "register_types.h"

#include "core/object/class_db.h"
#include "core/config/engine.h"

#include "src/rendering_weaver.h"
#include "src/resources/weaver_slot.h"
#include "src/resources/weaver_web.h"
#include "src/resources/weaver_spider.h"
#include "src/resources/weaver_web_node.h"

#include "src/weaver_spinner.h"
#include "src/weaver_view_anchor.h"
#include "src/weaver_spider_nest.h"

#ifdef TOOLS_ENABLED
#include "core/config/project_settings.h"
#include "editor/plugins/editor_plugin.h"
#include "src/editor/weaver_slot_editor_plugin.h"
#include "src/editor/weaver_slot_library.h"
#endif

void initialize_godot_weave_rd_module(ModuleInitializationLevel p_level)
{
	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS)
	{
		memnew(RenderingWeaver);
		ClassDB::register_class<RenderingWeaver>();
		Engine::get_singleton()->add_singleton(Engine::Singleton("RenderingWeaver", RenderingWeaver::get_singleton()));
		return;
	}

	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE)
	{
		ClassDB::register_abstract_class<WeaverSlot>();
		ClassDB::register_class<WeaverViewSlot>();
		ClassDB::register_class<WeaverCameraSlot>();

		ClassDB::register_class<WeaverWebNode>();
		ClassDB::register_class<WeaverSpider>();
		ClassDB::register_class<WeaverView>();
		ClassDB::register_class<WeaverWeb>();

		ClassDB::register_class<WeaverSpinner>();
		ClassDB::register_class<WeaverViewAnchor>();
		ClassDB::register_class<WeaverSpiderNest>();

#ifdef TOOLS_ENABLED
		// Editor-only slot management. Registered at scene level so tools builds can use it from scripts and tests.
		GLOBAL_DEF_BASIC(PropertyInfo(Variant::STRING, "weaver/editor/slot_directory", PROPERTY_HINT_DIR), "res://weaver/slots/");
		ClassDB::register_class<WeaverSlotLibrary>();
#endif
		return;
	}

#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR)
	{
		ClassDB::register_internal_class<EditorPropertyWeaverSlot>();
		ClassDB::register_internal_class<WeaverSlotInspectorPlugin>();
		ClassDB::register_internal_class<WeaverSlotEditorPlugin>();
		EditorPlugins::add_by_type<WeaverSlotEditorPlugin>();
		return;
	}
#endif
}

void uninitialize_godot_weave_rd_module(ModuleInitializationLevel p_level)
{
	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS)
	{
		if (RenderingWeaver *weaver = RenderingWeaver::get_singleton())
		{
			Engine::get_singleton()->remove_singleton("RenderingWeaver");
			memdelete(weaver);
		}
		return;
	}
}
