#pragma once

#include "scene/main/node.h"
#include "resources/weaver_slot.h"

//
// Anchors the parent viewport in to a view of the active WeaverWeb (the view with the same slot).
//
class WeaverViewAnchor : public Node
{
	GDCLASS(WeaverViewAnchor, Node);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_slot(const Ref<WeaverViewSlot> &p_slot);
	Ref<WeaverViewSlot> get_slot() const;

	PackedStringArray get_configuration_warnings() const override;

private:
	Ref<WeaverViewSlot> slot;

	void _register();
};
