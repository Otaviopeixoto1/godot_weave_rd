#pragma once

#include "core/io/resource_uid.h"
#include "core/object/object.h"
#include "core/variant/typed_array.h"
#include "resources/weaver_slot.h"

class WeaverWeb;

//
// (TOOL build only)
// Editor-side management of Slot Resource files (see: "resources/weaver_slot.h")
// New slots are written to a visible folder (project setting weaver/editor/slot_directory).
// Slots are listed per web (or by type in the case of "slot pickers" inside a web).
//
class WeaverSlotLibrary : public Object
{
	GDCLASS(WeaverSlotLibrary, Object);

protected:
	static void _bind_methods();

public:
	enum Kind
	{
		KIND_VIEW,
		KIND_CAMERA,
	};

	static String get_slot_directory();
	static String get_display_name(const Ref<WeaverSlot> &p_slot);
	static Kind get_kind(const Ref<WeaverSlot> &p_slot);

	// Saves a new slot to the slot directory as "<name>.view.tres" / "<name>.camera.tres" (suffix on collision).
	static Ref<WeaverSlot> create_slot(Kind p_kind, const String &p_name);

	// Updates resource_name and renames the file to match. References keep working through the file UID.
	static Error rename_slot(const Ref<WeaverSlot> &p_slot, const String &p_name);

	static PackedStringArray find_webs();
	static PackedStringArray find_slot_files(Kind p_kind);
	static PackedStringArray find_webs_using_slot(const Ref<WeaverSlot> &p_slot);

	// Slot files (anywhere in the project) that no saved WeaverWeb references.
	static PackedStringArray find_unused_slots();

	// Moves the files to the OS trash (falls back to deleting them).
	static Error delete_slot_files(const PackedStringArray &p_paths);

	// Every saved web with its slots: [{ path, web, views: [{ slot, lanes: [camera slot, ...] }] }]
	static Array collect_web_slots();

private:
	static String _file_suffix(Kind p_kind);
	static String _unique_path(Kind p_kind, const String &p_name, const String &p_ignore_path);
	static void _find_files(const String &p_dir, const StringName &p_base_type, PackedStringArray &r_paths);
	static void _ensure_uid(const String &p_path, ResourceUID::ID p_uid);
	static void _notify_filesystem(const String &p_path);
};

VARIANT_ENUM_CAST(WeaverSlotLibrary::Kind);
