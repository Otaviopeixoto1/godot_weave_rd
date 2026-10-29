#include "weaver_view.h"

#include "core/object/callable_mp.h"



void WeaverView::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_slot", "slot"), &WeaverView::set_slot);
	ClassDB::bind_method(D_METHOD("get_slot"), &WeaverView::get_slot);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "slot", PROPERTY_HINT_RESOURCE_TYPE, "WeaverViewSlot"), "set_slot", "get_slot");

	ClassDB::bind_method(D_METHOD("set_weaver_spiders", "weaver_spiders"), &WeaverView::set_weaver_spiders);
	ClassDB::bind_method(D_METHOD("get_weaver_spiders"), &WeaverView::get_weaver_spiders);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "weaver_spiders", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("WeaverSpider")), "set_weaver_spiders", "get_weaver_spiders");
}


void WeaverView::set_slot(const Ref<WeaverViewSlot> &p_slot) {
	slot = p_slot;
	emit_changed();
}

Ref<WeaverViewSlot> WeaverView::get_slot() const {
	return slot;
}

void WeaverView::set_weaver_spiders(const TypedArray<WeaverSpider> &p_weaver_spiders)
{
	const Callable forward_changed = callable_mp((Resource *)this, &Resource::emit_changed);

	// Reference counted connections, so a spider listed twice stays connected until its last entry is removed.
	for (const Ref<WeaverSpider> &weaver_spider : weaver_spiders)
	{
		if (weaver_spider.is_valid()) weaver_spider->disconnect_changed(forward_changed);
	}
	weaver_spiders.clear();

	for (int i = 0; i < p_weaver_spiders.size(); i++)
	{
		Ref<WeaverSpider> weaver_spider = p_weaver_spiders[i];
		weaver_spiders.push_back(weaver_spider);

		if (weaver_spider.is_valid()) weaver_spider->connect_changed(forward_changed, CONNECT_REFERENCE_COUNTED);
	}

	emit_changed();
}

TypedArray<WeaverSpider> WeaverView::get_weaver_spiders() const
{
	TypedArray<WeaverSpider> arr;

	for (uint32_t i = 0; i < weaver_spiders.size(); i++)
	{
		arr.push_back(weaver_spiders[i]);
	}

	return arr;
}
