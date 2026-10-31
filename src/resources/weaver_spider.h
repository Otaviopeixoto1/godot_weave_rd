#pragma once

#include "scene/resources/compositor.h"
#include "weaver_web_node.h"
#include "weaver_slot.h"

#include "core/variant/typed_array.h"
#include "core/object/object.h"
#include "core/object/class_db.h"

class WeaverSpider : public Resource
{
	GDCLASS(WeaverSpider, Resource);

protected:
	static void _bind_methods();
public:
	WeaverSpider() = default;
	virtual ~WeaverSpider() = default;

	void set_slot(const Ref<WeaverCameraSlot> &p_slot);
	Ref<WeaverCameraSlot> get_slot() const;

	// Entries are WeaverWebNodes or plain CompositorEffects. Null entries are kept so the inspector can add rows.
	void set_weaver_nodes(const TypedArray<Resource> &p_weaver_nodes);
	TypedArray<Resource> get_weaver_nodes() const;

private:
	Ref<WeaverCameraSlot> slot;
	LocalVector<Ref<Resource>> weaver_nodes;
};
