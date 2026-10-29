#include "weaver_slot_editor_plugin.h"
#include "resources/weaver_spider.h"
#include "resources/weaver_view.h"
#include "weaver_spider_nest.h"
#include "weaver_view_anchor.h"

#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "editor/editor_interface.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_toaster.h"
#include "editor/themes/editor_scale.h"
#include "scene/3d/camera_3d.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/tree.h"
#include "scene/main/viewport.h"


//
// EditorPropertyWeaverSlot
//
EditorPropertyWeaverSlot::EditorPropertyWeaverSlot()
{
	button = memnew(Button);
	button->set_h_size_flags(SIZE_EXPAND_FILL);
	button->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	button->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
	button->connect(SceneStringName(pressed), callable_mp(this, &EditorPropertyWeaverSlot::_button_pressed));
	add_child(button);
	add_focusable(button);

	menu = memnew(PopupMenu);
	menu->connect(SceneStringName(id_pressed), callable_mp(this, &EditorPropertyWeaverSlot::_menu_id_pressed));
	add_child(menu);

	name_dialog = memnew(ConfirmationDialog);
	name_edit = memnew(LineEdit);
	name_edit->set_placeholder("Slot name");
	name_edit->connect(SceneStringName(text_changed), callable_mp(this, &EditorPropertyWeaverSlot::_name_text_changed));
	name_dialog->add_child(name_edit);
	name_dialog->register_text_enter(name_edit);
	name_dialog->connect(SceneStringName(confirmed), callable_mp(this, &EditorPropertyWeaverSlot::_name_confirmed));
	add_child(name_dialog);
}

void EditorPropertyWeaverSlot::setup(WeaverSlotLibrary::Kind p_kind, Source p_source)
{
	kind = p_kind;
	source = p_source;
}

void EditorPropertyWeaverSlot::_notification(int p_what)
{
	if (p_what == NOTIFICATION_THEME_CHANGED)
	{
		button->set_button_icon(get_editor_theme_icon(kind == WeaverSlotLibrary::KIND_CAMERA ? SNAME("Camera3D") : SNAME("SubViewport")));
	}
}

void EditorPropertyWeaverSlot::_set_read_only(bool p_read_only)
{
	button->set_disabled(p_read_only);
}

Ref<WeaverSlot> EditorPropertyWeaverSlot::_get_slot()
{
	return get_edited_property_value();
}

void EditorPropertyWeaverSlot::update_property()
{
	const Ref<WeaverSlot> slot = _get_slot();
	button->set_text(WeaverSlotLibrary::get_display_name(slot));
	button->remove_theme_color_override(SceneStringName(font_color));

	String tooltip;
	if (slot.is_null())
	{
		tooltip = "No slot. Pick one from the menu.";
		button->add_theme_color_override(SceneStringName(font_color), get_theme_color(SNAME("warning_color"), EditorStringName(Editor)));
	}
	else if (slot->is_built_in())
	{
		tooltip = "This slot is not saved to its own file, so scenes and webs can't share it.";
		button->add_theme_color_override(SceneStringName(font_color), get_theme_color(SNAME("warning_color"), EditorStringName(Editor)));
	}
	else
	{
		const PackedStringArray webs = WeaverSlotLibrary::find_webs_using_slot(slot);
		tooltip = slot->get_path();
		if (webs.is_empty())
		{
			tooltip += "\nNot used by any saved WeaverWeb.";
			if (source == SOURCE_WEBS)
			{
				button->add_theme_color_override(SceneStringName(font_color), get_theme_color(SNAME("warning_color"), EditorStringName(Editor)));
			}
		}
		else
		{
			tooltip += "\nUsed by: " + String(", ").join(webs);
		}
	}
	button->set_tooltip_text(tooltip);
}

PopupMenu *EditorPropertyWeaverSlot::_new_submenu(PopupMenu *p_parent)
{
	PopupMenu *submenu = memnew(PopupMenu);
	submenu->connect(SceneStringName(id_pressed), callable_mp(this, &EditorPropertyWeaverSlot::_menu_id_pressed));
	p_parent->add_child(submenu);
	return submenu;
}

void EditorPropertyWeaverSlot::_add_slot_item(PopupMenu *p_menu, const Ref<WeaverSlot> &p_slot, const String &p_label)
{
	slot_paths.push_back(p_slot->get_path());
	p_menu->add_radio_check_item(p_label, ID_SLOT_BASE + slot_paths.size() - 1);
	p_menu->set_item_checked(-1, p_slot == _get_slot());
	if (p_slot->is_built_in())
	{
		p_menu->set_item_disabled(-1, true);
		p_menu->set_item_tooltip(-1, "Not saved to its own file.");
	}
}

// The view slot anchored in the viewport of the nest's camera (WeaverSpiderNest only).
Ref<WeaverSlot> EditorPropertyWeaverSlot::_get_anchor_slot_of_camera()
{
	const Node *nest = Object::cast_to<WeaverSpiderNest>(get_edited_object());
	const Camera3D *camera = nest ? Object::cast_to<Camera3D>(nest->get_parent()) : nullptr;
	Viewport *viewport = camera && camera->is_inside_tree() ? camera->get_viewport() : nullptr;
	if (viewport == nullptr) { return Ref<WeaverSlot>(); }

	List<Node *> queue;
	queue.push_back(viewport);
	while (!queue.is_empty())
	{
		Node *node = queue.front()->get();
		queue.pop_front();
		if (const WeaverViewAnchor *anchor = Object::cast_to<WeaverViewAnchor>(node))
		{
			if (anchor->get_slot().is_valid()) { return anchor->get_slot(); }
		}
		for (int i = 0; i < node->get_child_count(); i++)
		{
			Node *child = node->get_child(i);
			// Nested viewports belong to other views.
			if (!Object::cast_to<Viewport>(child)) { queue.push_back(child); }
		}
	}
	return Ref<WeaverSlot>();
}

void EditorPropertyWeaverSlot::_build_menu()
{
	menu->clear();
	slot_paths.clear();
	for (int i = menu->get_child_count() - 1; i >= 0; i--)
	{
		if (PopupMenu *child = Object::cast_to<PopupMenu>(menu->get_child(i)))
		{
			menu->remove_child(child);
			child->queue_free();
		}
	}

	const Ref<WeaverSlot> current = _get_slot();

	if (source == SOURCE_FILES)
	{
		const PackedStringArray files = WeaverSlotLibrary::find_slot_files(kind);
		for (const String &path : files)
		{
			const Ref<WeaverSlot> slot = ResourceLoader::load(path);
			if (slot.is_valid()) { _add_slot_item(menu, slot, WeaverSlotLibrary::get_display_name(slot)); }
		}
		if (files.is_empty())
		{
			menu->add_item("No slot files yet");
			menu->set_item_disabled(-1, true);
		}
		menu->add_separator();
		menu->add_icon_item(get_editor_theme_icon(SNAME("Add")), "New slot...", ID_NEW);
		if (current.is_valid())
		{
			menu->add_icon_item(get_editor_theme_icon(SNAME("Rename")), "Rename...", ID_RENAME);
		}
	}
	else
	{
		const Array webs = WeaverSlotLibrary::collect_web_slots();
		const Ref<WeaverSlot> anchor_slot = kind == WeaverSlotLibrary::KIND_CAMERA ? _get_anchor_slot_of_camera() : Ref<WeaverSlot>();

		// Lanes of the view anchored in the camera's own viewport come first.
		if (anchor_slot.is_valid())
		{
			menu->add_separator("In " + WeaverSlotLibrary::get_display_name(anchor_slot) + " (this camera's viewport)");
			HashSet<Ref<WeaverSlot>> added;
			for (int w = 0; w < webs.size(); w++)
			{
				const Array views = Dictionary(webs[w])["views"];
				for (int v = 0; v < views.size(); v++)
				{
					const Dictionary view = views[v];
					if (Ref<WeaverSlot>(view["slot"]) != anchor_slot) { continue; }
					const Array lanes = view["lanes"];
					for (int l = 0; l < lanes.size(); l++)
					{
						const Ref<WeaverSlot> lane_slot = lanes[l];
						if (lane_slot.is_null() || added.has(lane_slot)) { continue; }
						added.insert(lane_slot);
						_add_slot_item(menu, lane_slot, WeaverSlotLibrary::get_display_name(lane_slot));
					}
				}
			}
			menu->add_separator("All webs");
		}

		for (int w = 0; w < webs.size(); w++)
		{
			const Dictionary web = webs[w];
			const Array views = web["views"];
			PopupMenu *web_menu = _new_submenu(menu);
			for (int v = 0; v < views.size(); v++)
			{
				const Dictionary view = views[v];
				const Ref<WeaverSlot> view_slot = view["slot"];
				if (kind == WeaverSlotLibrary::KIND_VIEW)
				{
					if (view_slot.is_valid()) { _add_slot_item(web_menu, view_slot, WeaverSlotLibrary::get_display_name(view_slot)); }
					continue;
				}

				const Array lanes = view["lanes"];
				PopupMenu *view_menu = _new_submenu(web_menu);
				for (int l = 0; l < lanes.size(); l++)
				{
					const Ref<WeaverSlot> lane_slot = lanes[l];
					if (lane_slot.is_valid()) { _add_slot_item(view_menu, lane_slot, WeaverSlotLibrary::get_display_name(lane_slot)); }
				}
				String view_label = view_slot.is_valid() ? WeaverSlotLibrary::get_display_name(view_slot) : "View " + itos(v) + " (no slot)";
				if (anchor_slot.is_valid() && view_slot == anchor_slot) { view_label += "  (this camera's viewport)"; }
				if (view_menu->get_item_count() > 0)
				{
					web_menu->add_submenu_node_item(view_label, view_menu);
				}
				else
				{
					web_menu->remove_child(view_menu);
					memdelete(view_menu);
				}
			}

			if (web_menu->get_item_count() > 0)
			{
				menu->add_submenu_node_item(String(web["path"]).get_file().get_basename(), web_menu);
				menu->set_item_tooltip(-1, web["path"]);
			}
			else
			{
				menu->remove_child(web_menu);
				memdelete(web_menu);
			}
		}
		if (webs.is_empty())
		{
			menu->add_item("No saved WeaverWeb found");
			menu->set_item_disabled(-1, true);
		}
	}

	menu->add_separator();
	if (current.is_valid() && !current->is_built_in())
	{
		menu->add_icon_item(get_editor_theme_icon(SNAME("Filesystem")), "Show in FileSystem", ID_SHOW);
	}
	menu->add_icon_item(get_editor_theme_icon(SNAME("Clear")), "Clear", ID_CLEAR);
}

void EditorPropertyWeaverSlot::_button_pressed()
{
	_build_menu();
	const Rect2 rect = button->get_screen_rect();
	menu->reset_size();
	menu->set_position(rect.position + Vector2(0, rect.size.y));
	menu->set_min_size(Size2(rect.size.x, 0) / menu->get_content_scale_factor());
	menu->popup();
}

void EditorPropertyWeaverSlot::_menu_id_pressed(int p_id)
{
	switch (p_id)
	{
		case ID_CLEAR:
			emit_changed(get_edited_property(), Variant());
			break;
		case ID_NEW:
			_open_name_dialog(true);
			break;
		case ID_RENAME:
			_open_name_dialog(false);
			break;
		case ID_SHOW: {
			const Ref<WeaverSlot> slot = _get_slot();
			if (slot.is_valid()) { EditorInterface::get_singleton()->select_file(slot->get_path()); }
		} break;
		default: {
			const int index = p_id - ID_SLOT_BASE;
			ERR_FAIL_INDEX(index, slot_paths.size());
			const Ref<WeaverSlot> slot = ResourceLoader::load(slot_paths[index]);
			ERR_FAIL_COND(slot.is_null());
			emit_changed(get_edited_property(), slot);
		} break;
	}
}

void EditorPropertyWeaverSlot::_open_name_dialog(bool p_create)
{
	name_dialog_creates = p_create;
	const String type = kind == WeaverSlotLibrary::KIND_CAMERA ? "camera" : "view";
	name_dialog->set_title(p_create ? "New " + type + " slot" : "Rename " + type + " slot");
	name_dialog->set_ok_button_text(p_create ? "Create" : "Rename");
	name_edit->set_text(p_create ? String() : WeaverSlotLibrary::get_display_name(_get_slot()));
	_name_text_changed(name_edit->get_text());
	name_dialog->popup_centered(Size2(320, 0) * EDSCALE);
	name_edit->grab_focus();
	name_edit->select_all();
}

void EditorPropertyWeaverSlot::_name_text_changed(const String &p_text)
{
	name_dialog->get_ok_button()->set_disabled(p_text.strip_edges().is_empty());
}

void EditorPropertyWeaverSlot::_name_confirmed()
{
	const String name = name_edit->get_text().strip_edges();
	if (name.is_empty()) { return; }

	if (name_dialog_creates)
	{
		const Ref<WeaverSlot> slot = WeaverSlotLibrary::create_slot(kind, name);
		if (slot.is_valid()) { emit_changed(get_edited_property(), slot); }
	}
	else
	{
		WeaverSlotLibrary::rename_slot(_get_slot(), name);
		update_property();
	}
}


//
// WeaverSlotInspectorPlugin
//
bool WeaverSlotInspectorPlugin::can_handle(Object *p_object)
{
	return Object::cast_to<WeaverViewAnchor>(p_object) || Object::cast_to<WeaverSpiderNest>(p_object) ||
		   Object::cast_to<WeaverView>(p_object) || Object::cast_to<WeaverSpider>(p_object);
}

bool WeaverSlotInspectorPlugin::parse_property(Object *p_object, const Variant::Type p_type, const String &p_path, const PropertyHint p_hint, const String &p_hint_text, const BitField<PropertyUsageFlags> p_usage, const bool p_wide)
{
	if (p_path != "slot") { return false; }

	const bool camera = Object::cast_to<WeaverSpiderNest>(p_object) || Object::cast_to<WeaverSpider>(p_object);
	const bool scene_node = Object::cast_to<Node>(p_object) != nullptr;

	EditorPropertyWeaverSlot *editor = memnew(EditorPropertyWeaverSlot);
	editor->setup(camera ? WeaverSlotLibrary::KIND_CAMERA : WeaverSlotLibrary::KIND_VIEW,
			scene_node ? EditorPropertyWeaverSlot::SOURCE_WEBS : EditorPropertyWeaverSlot::SOURCE_FILES);
	add_property_editor(p_path, editor);
	return true;
}


//
// WeaverSlotEditorPlugin
//
static const char *CLEAN_MENU_ITEM = "Weaver: Clean unused slots...";

WeaverSlotEditorPlugin::WeaverSlotEditorPlugin()
{
	Ref<WeaverSlotInspectorPlugin> inspector_plugin;
	inspector_plugin.instantiate();
	add_inspector_plugin(inspector_plugin);
}

void WeaverSlotEditorPlugin::_notification(int p_what)
{
	switch (p_what)
	{
		case NOTIFICATION_ENTER_TREE: {
			clean_dialog = memnew(ConfirmationDialog);
			clean_dialog->set_title("Clean unused slots");
			clean_dialog->set_ok_button_text("Move to trash");
			VBoxContainer *box = memnew(VBoxContainer);
			Label *label = memnew(Label);
			label->set_text("These slot files are not used by any saved WeaverWeb. Scenes may still reference them.\nChecked files are moved to the trash.");
			box->add_child(label);
			clean_tree = memnew(Tree);
			clean_tree->set_hide_root(true);
			clean_tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
			box->add_child(clean_tree);
			clean_dialog->add_child(box);
			clean_dialog->connect(SceneStringName(confirmed), callable_mp(this, &WeaverSlotEditorPlugin::_clean_confirmed));
			EditorInterface::get_singleton()->get_base_control()->add_child(clean_dialog);

			add_tool_menu_item(CLEAN_MENU_ITEM, callable_mp(this, &WeaverSlotEditorPlugin::_clean_unused_slots));
		} break;
		case NOTIFICATION_EXIT_TREE: {
			remove_tool_menu_item(CLEAN_MENU_ITEM);
			if (clean_dialog)
			{
				clean_dialog->queue_free();
				clean_dialog = nullptr;
				clean_tree = nullptr;
			}
		} break;
	}
}

void WeaverSlotEditorPlugin::_clean_unused_slots()
{
	const PackedStringArray unused = WeaverSlotLibrary::find_unused_slots();
	if (unused.is_empty())
	{
		EditorToaster::get_singleton()->popup_str("No unused slot files.");
		return;
	}

	clean_tree->clear();
	TreeItem *root = clean_tree->create_item();
	for (const String &path : unused)
	{
		TreeItem *item = clean_tree->create_item(root);
		item->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
		item->set_editable(0, true);
		item->set_checked(0, true);
		item->set_text(0, path);
	}
	clean_dialog->popup_centered(Size2(560, 320) * EDSCALE);
}

void WeaverSlotEditorPlugin::_clean_confirmed()
{
	PackedStringArray paths;
	for (TreeItem *item = clean_tree->get_root()->get_first_child(); item; item = item->get_next())
	{
		if (item->is_checked(0)) { paths.push_back(item->get_text(0)); }
	}
	WeaverSlotLibrary::delete_slot_files(paths);
}
