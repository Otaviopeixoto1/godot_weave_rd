#pragma once

#include "scene/resources/compositor.h"


//
// TODO: Add logic for this. What do we share between nodes ? what can be shared ? how to better organze ?
// -One node per injection point or several injection points (several CompositorEffects) per node ?
// -Camera draw order might also be important
//
// -for a clear IO structure we need this to be one injection point per node. NODES HAVE TO BE ATOMIC. THEY HAVE INPUT AND OUTPUT AND THAT IS IT
// -WE CAN HAVE A NEW GRAPH AT THE WeaverSpider LEVEL, SINCE ALL RESOURCES ARE EASILY SHARED FOR THE SAME SPIDER (camera)
//
class WeaverWebNode : public CompositorEffect
{
	GDCLASS(WeaverWebNode, CompositorEffect);

	// TODO: Create a runtime script that will run the _render_callback from gdscript calling the WeaverWebNode::_internal_weaver_render_callback method
protected:
	void _notification(int p_what);
	static void _bind_methods();

	// Method called from script to reproduce a virtual c++ call.
	// THIS IS A FALLBACK SOLUTION since godot doesnt expose the _render_callback to modules yet...
	void _internal_weaver_render_callback(int p_effect_callback_type, const RenderData *p_render_data); // Replace with Variant arguments ???

public:
	WeaverWebNode();
	virtual ~WeaverWebNode();

	void set_spider(RID p_spider);
	virtual RID get_rid() const override { return node; }

private:
	RID node;
	RID spider;

	void _attach_script();
};
