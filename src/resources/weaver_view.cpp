#include "weaver_view.h"
#include "weaver_web.h"

#include "../rendering_weaver.h"


void WeaverView::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_weaver_spiders", "weaver_spiders"), &WeaverView::set_weaver_spiders);
	ClassDB::bind_method(D_METHOD("get_weaver_spiders"), &WeaverView::get_weaver_spiders);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "weaver_spiders", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("WeaverSpider")), "set_weaver_spiders", "get_weaver_spiders");
}

WeaverView::WeaverView()
{
	RenderingWeaver *weaver = RenderingWeaver::get_singleton();
	if (weaver != nullptr)
	{
		view = weaver->view_create();
	}
}

WeaverView::~WeaverView()
{
	RenderingWeaver *weaver = RenderingWeaver::get_singleton();
	if (weaver != nullptr && view.is_valid())
	{
		weaver->free_rid(view);
	}
}

void WeaverView::_set_web(RID p_web) {
	//TODO: trigger the signal for the node containing this WeaverView resource to signal to RenderingWeaver that its active
	//   ----> MAYBE NOT NECESSARY SINCE WE CAN USE THAT ALL RESOURCES INITIALIZE BEFORE THE SCENE (???)
	web = p_web;
}

void WeaverView::set_weaver_spiders(const TypedArray<WeaverSpider> &p_weaver_spiders)
{
	print_line("WEAVERVIEW SET SPIDERS");
	Vector<RID> spider_rids;
	weaver_spiders.clear();

	for (int i = 0; i < p_weaver_spiders.size(); i++)
	{
		Ref<WeaverSpider> weaver_spider = p_weaver_spiders[i];

		// We add the effect even if this is an empty Ref, this allows the UI to add new entries.
		weaver_spiders.push_back(weaver_spider);

		// But we only add a rid for valid Refs
		if (weaver_spider.is_valid())
		{
			RID rid = weaver_spider->get_rid();

			// Hydrate all new WeaverSpider with this WeaverView reference
			weaver_spider->set_view(view);
			print_line(vformat("VIEW HYDRATE SPIDER %s", rid));

			spider_rids.push_back(rid);
		}
	}

	RenderingWeaver::get_singleton()->view_set_spiders(view, spider_rids);
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
