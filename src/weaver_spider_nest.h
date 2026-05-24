#pragma once

#include "scene/main/node.h"
#include "resources/weaver_spider.h"

class WeaverSpiderNest : public Node
{
	GDCLASS(WeaverSpiderNest, Node);

	//
	//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// --------> Best Idea: Make FrameWeaver connect to its parent viewport and check if the camera has weaver_node_compositor assigned. If so then we compile the graph.
	// ---------- Register all active FrameWeavers to RenderingWeaver (Server) at runtime and dynamically compile dependencies
	// ---------- We must match Each WeaverWeb (from FrameWeaver) with active WeaverWebCompositors (from Camera3D/WorldEnvironment)
	// ---------- Make the plugin compile the scene only once and allow for easily assigning WeaverWebCompositors
	//
	// There are some implicit guarantees: There can be only one compositor (WeaverWebCompositor) per camera on the scene. These will be isolated nodes on the WeaverWeb.
	// We can compile WeaverWebCompositors statically and then the whole Web can be connected at runtime through signals between resources
	//
	// AT RUNTIME, INJECT THE Compositor into the camera. Setup vanilla passes as well as WeaverPasses
	//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

protected:
	void _notification(int p_what);
	static void _bind_methods();
public:
	void set_spider(const Ref<WeaverSpider> &p_spider);
	Ref<WeaverSpider> get_spider() const;

private:
	Ref<WeaverSpider> spider;
};
