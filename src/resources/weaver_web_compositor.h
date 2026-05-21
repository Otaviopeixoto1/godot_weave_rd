#pragma once

#include "scene/resources/compositor.h"

//Todo: Override the methods used to add effects here. dont display them on the property menu...
// Instead we will show the list with fixed weaver passes that can be configured in the frameWeaver or using the plugin as well
// as the list of raw CompositorEffects that can be added as well 
class WeaverWebCompositor : public Compositor
{
	GDCLASS(WeaverWebCompositor, Compositor);

	// All active WeaverCompositor must be declared in a WeaverWeb. 

protected:
	static void _bind_methods();
public:
private:
};
