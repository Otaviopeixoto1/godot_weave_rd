#pragma once

#include "scene/main/node.h"
#include "resources/weaver_spider.h"

//
// Binds its parent Camera3D to the lane (WeaverSpider) with the same WeaverCameraSlot (in the view anchored to the camera parent viewport)
//
class WeaverSpiderNest : public Node
{
	GDCLASS(WeaverSpiderNest, Node);
protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_slot(const Ref<WeaverCameraSlot> &p_slot);
	Ref<WeaverCameraSlot> get_slot() const;

	PackedStringArray get_configuration_warnings() const override;

private:
	Ref<WeaverCameraSlot> slot;

	void _register();
};
