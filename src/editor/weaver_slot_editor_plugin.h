#pragma once

#include "editor/inspector/editor_inspector.h"
#include "editor/plugins/editor_plugin.h"
#include "weaver_slot_library.h"

class Button;
class ConfirmationDialog;
class LineEdit;
class PopupMenu;
class Tree;

//
// Replaces the resource picker of a WeaverSlot  property with a slot menu.
//
// Scene nodes (WeaverViewAnchor / WeaverSpiderNest) pick from every saved WeaverWeb, grouped web > view > lane.
// Web parts (WeaverView / WeaverSpider) pick from every slot file of their type, and can create or rename slots.
//
class EditorPropertyWeaverSlot : public EditorProperty
{
	GDCLASS(EditorPropertyWeaverSlot, EditorProperty);

public:
	enum Source
	{
		SOURCE_WEBS,  // grouped by saved web (scene nodes)
		SOURCE_FILES, // every slot file of the kind (inside a web)
	};

	void setup(WeaverSlotLibrary::Kind p_kind, Source p_source);
	void update_property() override;

	EditorPropertyWeaverSlot();

protected:
	void _notification(int p_what);
	void _set_read_only(bool p_read_only) override;

private:
	enum MenuId
	{
		ID_CLEAR,
		ID_NEW,
		ID_RENAME,
		ID_SHOW,
		ID_SLOT_BASE = 100,
	};

	WeaverSlotLibrary::Kind kind = WeaverSlotLibrary::KIND_VIEW;
	Source source = SOURCE_WEBS;

	Button *button = nullptr;
	PopupMenu *menu = nullptr;
	ConfirmationDialog *name_dialog = nullptr;
	LineEdit *name_edit = nullptr;
	bool name_dialog_creates = false;

	Vector<String> slot_paths; // menu id - ID_SLOT_BASE -> slot file

	Ref<WeaverSlot> _get_slot();
	Ref<WeaverSlot> _get_anchor_slot_of_camera();
	PopupMenu *_new_submenu(PopupMenu *p_parent);
	void _add_slot_item(PopupMenu *p_menu, const Ref<WeaverSlot> &p_slot, const String &p_label);
	void _build_menu();
	void _button_pressed();
	void _menu_id_pressed(int p_id);
	void _open_name_dialog(bool p_create);
	void _name_text_changed(const String &p_text);
	void _name_confirmed();
};

class WeaverSlotInspectorPlugin : public EditorInspectorPlugin
{
	GDCLASS(WeaverSlotInspectorPlugin, EditorInspectorPlugin);

public:
	bool can_handle(Object *p_object) override;
	bool parse_property(Object *p_object, const Variant::Type p_type, const String &p_path, const PropertyHint p_hint, const String &p_hint_text, const BitField<PropertyUsageFlags> p_usage, const bool p_wide = false) override;
};

//
// Registers the slot inspector and the "Clean unused slots" tool.
//
class WeaverSlotEditorPlugin : public EditorPlugin
{
	GDCLASS(WeaverSlotEditorPlugin, EditorPlugin);

public:
	String get_plugin_name() const override { return "WeaverSlots"; }

	WeaverSlotEditorPlugin();

protected:
	void _notification(int p_what);

private:
	ConfirmationDialog *clean_dialog = nullptr;
	Tree *clean_tree = nullptr;

	void _clean_unused_slots();
	void _clean_confirmed();
};
