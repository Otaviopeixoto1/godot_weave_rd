#include "weaver_slot_library.h"
#include "resources/weaver_spider.h"
#include "resources/weaver_view.h"
#include "resources/weaver_web.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/io/resource_uid.h"
#include "core/object/class_db.h"
#include "core/os/os.h"
#include "core/templates/hash_set.h"
#include "editor/file_system/editor_file_system.h"

static const char *SLOT_DIRECTORY_SETTING = "weaver/editor/slot_directory";
static const char *SLOT_DIRECTORY_DEFAULT = "res://weaver/slots/";


void WeaverSlotLibrary::_bind_methods()
{
	ClassDB::bind_static_method("WeaverSlotLibrary", D_METHOD("get_slot_directory"), &WeaverSlotLibrary::get_slot_directory);
	ClassDB::bind_static_method("WeaverSlotLibrary", D_METHOD("get_display_name", "slot"), &WeaverSlotLibrary::get_display_name);
	ClassDB::bind_static_method("WeaverSlotLibrary", D_METHOD("get_kind", "slot"), &WeaverSlotLibrary::get_kind);
	ClassDB::bind_static_method("WeaverSlotLibrary", D_METHOD("create_slot", "kind", "name"), &WeaverSlotLibrary::create_slot);
	ClassDB::bind_static_method("WeaverSlotLibrary", D_METHOD("rename_slot", "slot", "name"), &WeaverSlotLibrary::rename_slot);
	ClassDB::bind_static_method("WeaverSlotLibrary", D_METHOD("find_webs"), &WeaverSlotLibrary::find_webs);
	ClassDB::bind_static_method("WeaverSlotLibrary", D_METHOD("find_slot_files", "kind"), &WeaverSlotLibrary::find_slot_files);
	ClassDB::bind_static_method("WeaverSlotLibrary", D_METHOD("find_webs_using_slot", "slot"), &WeaverSlotLibrary::find_webs_using_slot);
	ClassDB::bind_static_method("WeaverSlotLibrary", D_METHOD("find_unused_slots"), &WeaverSlotLibrary::find_unused_slots);
	ClassDB::bind_static_method("WeaverSlotLibrary", D_METHOD("delete_slot_files", "paths"), &WeaverSlotLibrary::delete_slot_files);
	ClassDB::bind_static_method("WeaverSlotLibrary", D_METHOD("collect_web_slots"), &WeaverSlotLibrary::collect_web_slots);

	BIND_ENUM_CONSTANT(KIND_VIEW);
	BIND_ENUM_CONSTANT(KIND_CAMERA);
}

String WeaverSlotLibrary::get_slot_directory()
{
	String dir = SLOT_DIRECTORY_DEFAULT;
	if (ProjectSettings::get_singleton()->has_setting(SLOT_DIRECTORY_SETTING))
	{
		dir = GLOBAL_GET(SLOT_DIRECTORY_SETTING);
	}
	if (dir.strip_edges().is_empty())
	{
		dir = SLOT_DIRECTORY_DEFAULT;
	}
	return dir.trim_suffix("/") + "/";
}

String WeaverSlotLibrary::get_display_name(const Ref<WeaverSlot> &p_slot)
{
	if (p_slot.is_null()) { return "No slot"; }
	if (!p_slot->get_name().is_empty()) { return p_slot->get_name(); }
	if (!p_slot->is_built_in())
	{
		return p_slot->get_path().get_file().trim_suffix(_file_suffix(get_kind(p_slot)));
	}

	return "(unsaved)";
}

WeaverSlotLibrary::Kind WeaverSlotLibrary::get_kind(const Ref<WeaverSlot> &p_slot)
{
	return Object::cast_to<WeaverCameraSlot>(p_slot.ptr()) ? KIND_CAMERA : KIND_VIEW;
}

Ref<WeaverSlot> WeaverSlotLibrary::create_slot(Kind p_kind, const String &p_name)
{
	const String name = p_name.strip_edges();
	ERR_FAIL_COND_V_MSG(name.is_empty(), Ref<WeaverSlot>(), "A slot needs a name.");

	const String dir = get_slot_directory();
	if (!DirAccess::exists(dir))
	{
		ERR_FAIL_COND_V_MSG(DirAccess::make_dir_recursive_absolute(dir) != OK, Ref<WeaverSlot>(), "Can't create the slot directory " + dir);
	}

	Ref<WeaverSlot> slot;
	if (p_kind == KIND_CAMERA) { slot = Ref<WeaverSlot>(memnew(WeaverCameraSlot)); }
	else { slot = Ref<WeaverSlot>(memnew(WeaverViewSlot)); }

	slot->set_name(name);

	const String path = _unique_path(p_kind, name, String());
	// Owning the path makes the resource cache return this instance for the file.
	slot->set_path(path);
	ERR_FAIL_COND_V_MSG(ResourceSaver::save(slot, path) != OK, Ref<WeaverSlot>(), "Can't save the slot to " + path);
	_ensure_uid(path, ResourceUID::INVALID_ID);
	_notify_filesystem(path);
	return slot;
}

Error WeaverSlotLibrary::rename_slot(const Ref<WeaverSlot> &p_slot, const String &p_name)
{
	ERR_FAIL_COND_V(p_slot.is_null(), ERR_INVALID_PARAMETER);
	const String name = p_name.strip_edges();
	ERR_FAIL_COND_V_MSG(name.is_empty(), ERR_INVALID_PARAMETER, "A slot needs a name.");

	p_slot->set_name(name);
	if (p_slot->is_built_in())
	{
		// Unsaved or embedded: there is no file to rename.
		p_slot->emit_changed();
		return OK;
	}

	const String old_path = p_slot->get_path();
	const ResourceUID::ID uid = ResourceLoader::get_resource_uid(old_path);
	Error err = ResourceSaver::save(p_slot, old_path);
	ERR_FAIL_COND_V(err != OK, err);

	const String new_path = _unique_path(get_kind(p_slot), name, old_path);
	if (new_path != old_path)
	{
		err = DirAccess::rename_absolute(old_path, new_path);
		ERR_FAIL_COND_V_MSG(err != OK, err, "Can't rename " + old_path + " to " + new_path);
		p_slot->set_path(new_path, true);
		_notify_filesystem(old_path);
	}

	// References to the slot resolve through this UID, so it must survive the save and point at the new path.
	_ensure_uid(new_path, uid);
	_notify_filesystem(new_path);
	p_slot->emit_changed();
	return OK;
}

PackedStringArray WeaverSlotLibrary::find_webs()
{
	PackedStringArray paths;
	_find_files("res://", WeaverWeb::get_class_static(), paths);
	paths.sort();
	return paths;
}

PackedStringArray WeaverSlotLibrary::find_slot_files(Kind p_kind)
{
	PackedStringArray paths;
	_find_files("res://", p_kind == KIND_CAMERA ? WeaverCameraSlot::get_class_static() : WeaverViewSlot::get_class_static(), paths);
	paths.sort();
	return paths;
}

PackedStringArray WeaverSlotLibrary::find_webs_using_slot(const Ref<WeaverSlot> &p_slot)
{
	PackedStringArray result;
	if (p_slot.is_null()) { return result; }

	const Array webs = collect_web_slots();
	for (int w = 0; w < webs.size(); w++)
	{
		const Dictionary web = webs[w];
		const Array views = web["views"];
		bool used = false;
		for (int v = 0; v < views.size() && !used; v++)
		{
			const Dictionary view = views[v];
			used = Ref<WeaverSlot>(view["slot"]) == p_slot || Array(view["lanes"]).has(p_slot);
		}
		if (used) { result.push_back(web["path"]); }
	}
	return result;
}

PackedStringArray WeaverSlotLibrary::find_unused_slots()
{
	HashSet<String> used;
	const Array webs = collect_web_slots();
	for (int w = 0; w < webs.size(); w++)
	{
		const Array views = Dictionary(webs[w])["views"];
		for (int v = 0; v < views.size(); v++)
		{
			const Dictionary view = views[v];
			const Ref<WeaverSlot> view_slot = view["slot"];
			if (view_slot.is_valid()) { used.insert(view_slot->get_path()); }

			const Array lanes = view["lanes"];
			for (int l = 0; l < lanes.size(); l++)
			{
				const Ref<WeaverSlot> lane_slot = lanes[l];
				if (lane_slot.is_valid()) { used.insert(lane_slot->get_path()); }
			}
		}
	}

	PackedStringArray unused;
	PackedStringArray files = find_slot_files(KIND_VIEW);
	files.append_array(find_slot_files(KIND_CAMERA));
	for (const String &path : files)
	{
		if (!used.has(path)) { unused.push_back(path); }
	}

	unused.sort();
	return unused;
}

Error WeaverSlotLibrary::delete_slot_files(const PackedStringArray &p_paths)
{
	Error result = OK;
	for (const String &path : p_paths)
	{
		Error err = OS::get_singleton()->move_to_trash(ProjectSettings::get_singleton()->globalize_path(path));
		if (err != OK)
		{
			err = DirAccess::remove_absolute(path);
		}
		if (err != OK)
		{
			ERR_PRINT("Can't delete the slot file " + path);
			result = err;
			continue;
		}

		_notify_filesystem(path);
	}
	return result;
}

Array WeaverSlotLibrary::collect_web_slots()
{
	Array result;
	for (const String &path : find_webs())
	{
		const Ref<WeaverWeb> web = ResourceLoader::load(path);
		if (web.is_null()) { continue; }

		Array views;
		const TypedArray<WeaverView> web_views = web->get_weaver_views();
		for (int v = 0; v < web_views.size(); v++)
		{
			const Ref<WeaverView> view = web_views[v];
			if (view.is_null()) { continue; }

			Array lanes;
			const TypedArray<WeaverSpider> spiders = view->get_weaver_spiders();
			for (int l = 0; l < spiders.size(); l++)
			{
				const Ref<WeaverSpider> spider = spiders[l];
				lanes.push_back(spider.is_valid() ? Variant(spider->get_slot()) : Variant());
			}

			Dictionary entry;
			entry["slot"] = view->get_slot();
			entry["lanes"] = lanes;
			views.push_back(entry);
		}

		Dictionary entry;
		entry["path"] = path;
		entry["web"] = web;
		entry["views"] = views;
		result.push_back(entry);
	}
	return result;
}

String WeaverSlotLibrary::_file_suffix(Kind p_kind)
{
	return p_kind == KIND_CAMERA ? ".camera.tres" : ".view.tres";
}

String WeaverSlotLibrary::_unique_path(Kind p_kind, const String &p_name, const String &p_ignore_path)
{
	String base = p_name.validate_filename().replace(" ", "_");
	if (base.is_empty()) { base = "slot"; }

	const String dir = get_slot_directory();
	String path = dir + base + _file_suffix(p_kind);
	for (int n = 2; path != p_ignore_path && FileAccess::exists(path); n++)
	{
		path = dir + base + "_" + itos(n) + _file_suffix(p_kind);
	}
	return path;
}

static void _find_in_editor_dir(EditorFileSystemDirectory *p_dir, const StringName &p_base_type, PackedStringArray &r_paths)
{
	for (int i = 0; i < p_dir->get_file_count(); i++)
	{
		const StringName type = p_dir->get_file_type(i);
		if (type == p_base_type || ClassDB::is_parent_class(type, p_base_type))
		{
			r_paths.push_back(p_dir->get_file_path(i));
		}
	}
	for (int i = 0; i < p_dir->get_subdir_count(); i++)
	{
		_find_in_editor_dir(p_dir->get_subdir(i), p_base_type, r_paths);
	}
}

void WeaverSlotLibrary::_find_files(const String &p_dir, const StringName &p_base_type, PackedStringArray &r_paths)
{
	// The editor already knows every file's type; scanning by hand is only needed without an editor (headless tests).
	EditorFileSystem *efs = EditorFileSystem::get_singleton();
	if (efs != nullptr && efs->get_filesystem() != nullptr)
	{
		_find_in_editor_dir(efs->get_filesystem(), p_base_type, r_paths);
		return;
	}

	Ref<DirAccess> da = DirAccess::open(p_dir);
	if (da.is_null() || da->file_exists(".gdignore")) { return; }

	da->list_dir_begin();
	for (String file = da->get_next(); !file.is_empty(); file = da->get_next())
	{
		if (file.begins_with(".")) { continue; }

		const String path = p_dir.path_join(file);
		if (da->current_is_dir())
		{
			_find_files(path, p_base_type, r_paths);
			continue;
		}

		const String ext = file.get_extension().to_lower();
		if (ext != "tres" && ext != "res") { continue; }

		const String type = ResourceLoader::get_resource_type(path);
		if (type == p_base_type || ClassDB::is_parent_class(type, p_base_type))
		{
			r_paths.push_back(path);
		}
	}
	da->list_dir_end();
}

void WeaverSlotLibrary::_ensure_uid(const String &p_path, ResourceUID::ID p_uid)
{
	// The editor writes UIDs on save; without it (scripts, tests) a saved file has none, and references would break
	// on rename.
	ResourceUID::ID uid = ResourceLoader::get_resource_uid(p_path);
	if (uid == ResourceUID::INVALID_ID)
	{
		uid = p_uid != ResourceUID::INVALID_ID ? p_uid : ResourceUID::get_singleton()->create_id();
		ResourceSaver::set_uid(p_path, uid);
	}
	if (ResourceUID::get_singleton()->has_id(uid)) { ResourceUID::get_singleton()->set_id(uid, p_path); }
	else { ResourceUID::get_singleton()->add_id(uid, p_path); }
}

void WeaverSlotLibrary::_notify_filesystem(const String &p_path)
{
	if (EditorFileSystem::get_singleton() != nullptr)
	{
		EditorFileSystem::get_singleton()->update_file(p_path);
	}
}
