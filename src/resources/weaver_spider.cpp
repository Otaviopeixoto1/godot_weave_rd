#include "weaver_spider.h"

#include "core/object/callable_mp.h"


void WeaverSpider::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_slot", "slot"), &WeaverSpider::set_slot);
	ClassDB::bind_method(D_METHOD("get_slot"), &WeaverSpider::get_slot);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "slot", PROPERTY_HINT_RESOURCE_TYPE, "WeaverCameraSlot"), "set_slot", "get_slot");

	ClassDB::bind_method(D_METHOD("set_weaver_nodes", "weaver_nodes"), &WeaverSpider::set_weaver_nodes);
	ClassDB::bind_method(D_METHOD("get_weaver_nodes"), &WeaverSpider::get_weaver_nodes);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "weaver_nodes", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("WeaverWebNode,CompositorEffect")), "set_weaver_nodes", "get_weaver_nodes");
}


void WeaverSpider::set_slot(const Ref<WeaverCameraSlot> &p_slot) {
	slot = p_slot;
	emit_changed();
}

Ref<WeaverCameraSlot> WeaverSpider::get_slot() const {
	return slot;
}


void WeaverSpider::set_weaver_nodes(const TypedArray<Resource> &p_weaver_nodes)
{
	const Callable forward_changed = callable_mp((Resource *)this, &Resource::emit_changed);

	// Reference counted connections, so a node listed twice stays connected until its last entry is removed.
	for (const Ref<Resource> &weaver_node : weaver_nodes)
	{
		if (weaver_node.is_valid()) weaver_node->disconnect_changed(forward_changed);
	}
	weaver_nodes.clear();

	for (int i = 0; i < p_weaver_nodes.size(); i++)
	{
		Ref<Resource> weaver_node = p_weaver_nodes[i];

		if (weaver_node.is_valid() && !Object::cast_to<WeaverWebNode>(weaver_node.ptr()) && !Object::cast_to<CompositorEffect>(weaver_node.ptr()))
		{
			ERR_PRINT(vformat("WeaverSpider: entry %d is a %s, expected a WeaverWebNode or a CompositorEffect. It was cleared.", i, weaver_node->get_class()));
			weaver_node.unref();
		}

		weaver_nodes.push_back(weaver_node);

		if (weaver_node.is_valid()) weaver_node->connect_changed(forward_changed, CONNECT_REFERENCE_COUNTED);
	}

	emit_changed();
}

TypedArray<Resource> WeaverSpider::get_weaver_nodes() const
{
	TypedArray<Resource> arr;

	for (uint32_t i = 0; i < weaver_nodes.size(); i++) {
		arr.push_back(weaver_nodes[i]);
	}

	return arr;
}
