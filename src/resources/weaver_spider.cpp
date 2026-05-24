#include "weaver_spider.h"
#include "../rendering_weaver.h"


void WeaverSpider::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_weaver_nodes", "weaver_nodes"), &WeaverSpider::set_weaver_nodes);
	ClassDB::bind_method(D_METHOD("get_weaver_nodes"), &WeaverSpider::get_weaver_nodes);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "weaver_nodes", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("WeaverWebNode")), "set_weaver_nodes", "get_weaver_nodes");
}

WeaverSpider::WeaverSpider()
{
	RenderingWeaver *weaver = RenderingWeaver::get_singleton();
	if (weaver != nullptr)
	{
		spider = weaver->spider_create();
	}
}

WeaverSpider::~WeaverSpider()
{
	RenderingWeaver *weaver = RenderingWeaver::get_singleton();
	if (weaver != nullptr && spider.is_valid())
	{
		weaver->free_rid(spider);
	}
}

void WeaverSpider::set_view(RID p_view)
{
	//TODO: trigger the signal for the node containing this WeaverSpider resource to signal to RenderingWeaver that its active
	//   ----> MAYBE NOT NECESSARY SINCE WE CAN USE THAT ALL RESOURCES INITIALIZE BEFORE THE SCENE (???)
	view = p_view;
}

void WeaverSpider::set_weaver_nodes(const TypedArray<WeaverWebNode> &p_weaver_nodes)
{
	print_line("WEAVERSPIDER SET NODES");
	Vector<RID> node_rids;
	weaver_nodes.clear();

	for (int i = 0; i < p_weaver_nodes.size(); i++)
	{
		// Cast to proper ref, if our object isn't a CompositorEffect resource this will be an empty Ref.
		Ref<WeaverWebNode> weaver_node = p_weaver_nodes[i];

		// We add the effect even if this is an empty Ref, this allows the UI to add new entries.
		weaver_nodes.push_back(weaver_node);

		// But we only add a rid for valid Refs
		if (weaver_node.is_valid())
		{
			RID rid = weaver_node->get_rid();

			// Hydrate all new WeaverWebNode with this WeaverSpider reference
			weaver_node->set_spider(spider);
			print_line(vformat("SPIDER HYDRATE Node %s", rid));

			node_rids.push_back(rid);
		}
	}

	RenderingWeaver::get_singleton()->spider_set_nodes(spider, node_rids);
}

TypedArray<WeaverWebNode> WeaverSpider::get_weaver_nodes() const
{
	TypedArray<WeaverWebNode> arr;

	for (uint32_t i = 0; i < weaver_nodes.size(); i++) {
		arr.push_back(weaver_nodes[i]);
	}

	return arr;
}
