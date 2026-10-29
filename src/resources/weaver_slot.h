#pragma once

#include "core/io/resource.h"

//
// Identity tokens used to bind parts of a WeaverWeb to scene nodes.
// A slot names a role ("main camera", "minimap view") instead of a specific Web, so a scene that
// references a slot works with any Web that uses the same slot. Identity is the loaded instance:
// the resource cache returns one instance per path, so a slot must be saved to its own .tres file
// to be shared between Webs and scenes.
//
class WeaverSlot : public Resource
{
	GDCLASS(WeaverSlot, Resource);

protected:
	static void _bind_methods();

public:
	void set_description(const String &p_description);
	String get_description() const;

	// True only when the slot is saved to its own file (not unsaved and not embedded as a sub-resource).
	bool is_shareable() const;

private:
	String description;
};

// Identifies a view (a Viewport).
class WeaverViewSlot : public WeaverSlot
{
	GDCLASS(WeaverViewSlot, WeaverSlot);
};

// Identifies a lane (WeaverSpider) inside a view, bound to a camera.
class WeaverCameraSlot : public WeaverSlot
{
	GDCLASS(WeaverCameraSlot, WeaverSlot);
};
