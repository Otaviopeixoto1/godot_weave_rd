#pragma once

#include "scene/main/node.h"
#include "resources/weaver_web.h"

// This Node handles the runtime graph of the RenderingWeaver for an entire WeaverWeb.
class FrameWeaver : public Node
{
	GDCLASS(FrameWeaver, Node);

	//
	// This should replicate a WeaverWeb as a set of nodes at runtime. It should connect all WeaverCompositors that are set as its children
	// - We will connect to all WeaverCompositors in an intuitive way (maybe automatically ?)
	// - We then compile the graph at runtime and test dependencies
	//
	// ALL WeaverCompositor should be stored inside a WeaverWeb which should be assigned here !
	// Any scene changes have to trigger WeaverWeb recompilations should be done by this node
	// ----> Invert the logic: Make a virtual scene in the WeaverWeb. Load the nodes into the scene from weaverWeb references !!
	//       THIS IS THE SAFE AND EASY APPROACH: WE CAN LATER LET THE USER OVERRIDE THE GENERATED NODES BUT THEIR STRUCTURE SHOULD BE FIXED 
	//
	// ----> This node`s children will be used to make the graph structure:
	//     - We create viewports and cameras and freeze them using: node.set_meta("_edit_lock_", true)
	//     - We should DEFINE CUSTOM Viewport AND Camera3D classes for better management and to allow users to inject custom node classes
	//     -- One alternative for injection is to allow for the use of custom scenes with a specific root node (e.g. inheriting from Subviewport)
	//
	//
	//  A single web can spawn several viewports on the scene. This would only support a single scene setup but it should work well for now
	//	-The current design also removes the possibility of multiple viewports from different scenes having to be accounted for...
	//  -It also allows for easy reordering between viewports wich can be good since godot RenderGraph might not reorder them (?)
	//
	// --> THERE IS A MASSIVE PROBLEM: IF USER ADDS CUSTOM NODES TO SCENE, WHENEVER THE GRAPH GETS EDITED, ALL OF THEM ARE DELETED
	// ---- Locking nodes through node.set_meta("_edit_lock_", true) really doesnt work so well...
	//
	//
	// --------> Another Idea: The user creates nodes on the scene and then registers them to the WeaverWeb resource...
	// ---------- Node references in the scene are not stable ! (nodepaths are the more stable solution but they break too easy...)
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
	void set_web(const Ref<WeaverWeb> &p_web);
	Ref<WeaverWeb> get_web() const;

private:
	Ref<WeaverWeb> web;
};
