#pragma once

#include "core/object/object.h"

// Central manager class.
//TODO: Define WeaverPass as the framegraph pass 
class RenderingWeaver : public Object
{
	GDCLASS(RenderingWeaver, Object);
	//This should act as a global server. The single source of truth for all passes.
	// -All WeaverPasses can be shared but they will have restrictions based on injection points.
	// -This should be independent of the current loaded scene. ALL WEAVER PASSES MUST BE ACCESSIBLE AND REGISTERED IN THE RenderingWeaver
	// --We need to have specific viewport contexts. Viewport contexts should be accessible as well
	// -The RenderingWeaverPlugin should show all resources in a special window
	// --Viewport contexts need to be exposed in this window as well. We need to register viewports and somehow manage them in a scene independent manner
private:
	static RenderingWeaver *singleton;
	int value{ 0 };

protected:
	static void _bind_methods();
	void add(int value);

public:
	RenderingWeaver();
	static RenderingWeaver *get_singleton();
};
