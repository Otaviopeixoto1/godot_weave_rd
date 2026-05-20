#pragma once

#include "core/io/resource.h"
#include "weaver_node_compositor.h"

//Todo: Override the methods used to add effects here. dont display them on the property menu...
// Instead we will show the list with fixed weaver passes that can be configured in the frameWeaver or using the plugin as well
// as the list of raw CompositorEffects that can be added as well 
class WeaverWeb : public Resource
{
	GDCLASS(WeaverWeb, Resource);

protected:
	void _notification(int p_what);
	static void _bind_methods();
public:
	WeaverWeb();

	void set_weaver_nodes(const TypedArray<WeaverNodeCompositor> &p_weaver_nodes);
	TypedArray<WeaverNodeCompositor> get_weaver_nodes() const;

	//TODO: Add accessor to internal resource RIDs

private:
	LocalVector<Ref<WeaverNodeCompositor>> weaver_nodes;
	// At a higher level, we need to focus on compositors as our nodes since there are many restrictions in what can be shared between compositors,
	// while within compositors we are pretty much free to do all kinds of sharing

	//FIRST: Figure out how maintain the state between scene hierarchy and WeaverNodeCompositors ! (draw graph)
	// -Multiviewport and cross-scene compilation is the hardest
};
