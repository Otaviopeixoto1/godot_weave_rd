#pragma once

#include "core/object/object.h"

// Central manager class for all WeaverWebs .
//TODO: Define WeaverPass as the framegraph pass 
class RenderingWeaver : public Object
{
	GDCLASS(RenderingWeaver, Object);
	//
	// TODO:
	// -Given a scene we will detect and manage all compositors and compositor effects within it. We can make a WeaverContext resource for storing all used scenes and their interdependencies
	// -We will track WeaverCompositorEffects in this WeaverContext resource for each viewport and for each scene that is associated with it
	// --What really matters is the output of the viewport. Only one camera can be active per viewport so other cameras wont matter. WE HOOK INTO A VIEWPORT OUTPUT !
	// 
	// -We can give a node to the user for setting the active weaver context for all the scene (for now it should be exclusive like WorldEnvironment but it may work just like
	//  a viewport in the future)
	// --We use the plugin GraphEdit nodes to make it easy to edit the dependency graphs especially between scenes ! This will allow the creation and assignment of
	//   WeaverEffects to the scene Cameras and WorldEnvironments
	// --Each effect should have inputs and outputs. If an input is not satisfied (missing dependencies on the scene) it will not be used
	//
	//
	// 1) We need to have an intuitive way of overriding the active compositors:
	//  -We must override the camera compositor since they have priority and are more granular
	//
	// 2) No scene tracking (maybe only for the plugin to display where are the resources being attached):
	//  -We use other resources to represent messages (like Resource/ScriptableObject signal busses)
	//  -Dependencies will be based on these resources. Each effect will have inputs and outputs where we can assign those resources
	//  -Compile the graph at runtime and check if dependencies are setup. If they are not we disable the paths using them
	//
	// -> We still need to compile the graph ! we can use a PER-VIEWPORT REGISTRATION and dynamically compile !
	// -> Build a resource database that will only track these special WeaverResource types and later we can display them on the plugin !
	// -> For each pass we can declare input and output resources. We will reference those inputs and outputs inside other passes
	//
	// -> WE NEED TO HAVE A CUSTOM Compositor Resource class that will expose to the user the ability to ad normal passes as well as weaver passes and integrate with the custom viewports class
	// -Custom viewport is not a good idea for the state manager class -> MAKE A WeaverContext/Environment Node to store state. Similar to WorldEnvironment it shall only be one per scene
	//


private:
	static RenderingWeaver *singleton;

protected:
	static void _bind_methods();
	void add(int value);

public:
	RenderingWeaver();
	static RenderingWeaver *get_singleton();
};
