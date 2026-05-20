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
	//		
	//

protected:
	void _notification(int p_what);
	static void _bind_methods();
public:
	void set_web(const Ref<WeaverWeb> &p_web);
	Ref<WeaverWeb> get_web() const;

private:
	Ref<WeaverWeb> web;
};
