#pragma once

#include "scene/resources/compositor.h"


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
	WeaverWebNode() = default;

private:
	void _attach_script();
};
