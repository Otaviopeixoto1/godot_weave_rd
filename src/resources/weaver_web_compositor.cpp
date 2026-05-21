#include "weaver_web_compositor.h"



void WeaverWebCompositor::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_weaver_nodes", "weaver_nodes"), &WeaverWebCompositor::set_weaver_nodes);
	ClassDB::bind_method(D_METHOD("get_weaver_nodes"), &WeaverWebCompositor::get_weaver_nodes);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "weaver_nodes", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("WeaverWebNode")), "set_weaver_nodes", "get_weaver_nodes");
}

void WeaverWebCompositor::set_weaver_nodes(const TypedArray<WeaverWebNode> &p_weaver_nodes)
{
	//Array effect_rids;
	weaver_nodes.clear();

	for (int i = 0; i < p_weaver_nodes.size(); i++) {
		// Cast to proper ref, if our object isn't a CompositorEffect resource this will be an empty Ref.
		Ref<WeaverWebNode> weaver_node = p_weaver_nodes[i];

		// We add the effect even if this is an empty Ref, this allows the UI to add new entries.
		weaver_nodes.push_back(weaver_node);

		// But we only add a rid for valid Refs
		if (weaver_node.is_valid()) {
			//RID rid = compositor_effect->get_rid();
			//effect_rids.push_back(rid);
		}
	}
}

TypedArray<WeaverWebNode> WeaverWebCompositor::get_weaver_nodes() const {
	TypedArray<WeaverWebNode> arr;

	for (uint32_t i = 0; i < weaver_nodes.size(); i++) {
		arr.push_back(weaver_nodes[i]);
	}

	return arr;
}
