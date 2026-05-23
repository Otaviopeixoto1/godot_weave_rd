#include "weaver_web.h"



void WeaverWeb::_notification(int p_what)
{
	switch (p_what) {
		case NOTIFICATION_POSTINITIALIZE: {
			print_line("WEAVERWEB POSTINIT");

			//
			// 1st: The web must be initialized in the scene. Only a single web can be active at a time and the user may define multiple webs
			// 2nd: The web hydrates all dependent resources (views and compositors -> Views should own compositors)
			// 3rd: Each node holding a dependent resource sends a signal back to the web holder after check if the tree structure is correct (bottom-up traversal)
			// 4th: The web is dynamically compiled based on the nodes
			//


			//
			// Init all weaver views with a reference to this web. Then Init all compositors...
			// INSTEAD OF REFERENCES, MAKE RIDS USING THE RenderingWeaver server -> THIS ALLOWS FOR BETTER CHECKING, DECOUPLING AND HANDLING MULTIPLE ACTIVE WEBS
			// 

		} break;
	}
}

void WeaverWeb::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_weaver_views", "weaver_views"), &WeaverWeb::set_weaver_views);
	ClassDB::bind_method(D_METHOD("get_weaver_views"), &WeaverWeb::get_weaver_views);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "weaver_views", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("WeaverView")), "set_weaver_views", "get_weaver_views");
}


WeaverWeb::WeaverWeb()
{
	print_line("WEAVERWEB CONSTRUCT");
}

void WeaverWeb::set_weaver_views(const TypedArray<WeaverView> &p_weaver_views)
{
	//Array effect_rids;
	weaver_views.clear();

	for (int i = 0; i < p_weaver_views.size(); i++)
	{
		// Cast to proper ref, if our object isn't a CompositorEffect resource this will be an empty Ref.
		Ref<WeaverView> weaver_node = p_weaver_views[i];

		//
		// TODO: Hydrate all new WeaverViews with this WeaverWeb reference
		// INSTEAD OF REFERENCES, MAKE RIDS USING THE RenderingWeaver server !
		//

		// We add the effect even if this is an empty Ref, this allows the UI to add new entries.
		weaver_views.push_back(weaver_node);

		// But we only add a rid for valid Refs
		if (weaver_node.is_valid())
		{
			//RID rid = compositor_effect->get_rid();
			//effect_rids.push_back(rid);
		}
	}
}

TypedArray<WeaverView> WeaverWeb::get_weaver_views() const
{
	TypedArray<WeaverView> arr;

	for (uint32_t i = 0; i < weaver_views.size(); i++)
	{
		arr.push_back(weaver_views[i]);
	}

	return arr;
}
