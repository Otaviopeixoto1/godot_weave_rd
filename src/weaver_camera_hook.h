#pragma once

#include "scene/main/node.h"
#include "resources/weaver_web_compositor.h"

class WeaverCameraHook : public Node
{
	GDCLASS(WeaverCameraHook, Node);

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
	//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

protected:
	void _notification(int p_what);
	static void _bind_methods();
public:
	void set_web_compositor(const Ref<WeaverWebCompositor> &p_web);
	Ref<WeaverWebCompositor> get_web_compositor() const;

private:
	Ref<WeaverWebCompositor> web_compositor;
};
