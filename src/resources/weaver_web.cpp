#include "weaver_web.h"



void WeaverWeb::_notification(int p_what)
{
	switch (p_what) {
		case NOTIFICATION_POSTINITIALIZE: {
			print_line("WEAVERWEB POSTINIT");
		} break;
	}
}

void WeaverWeb::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_weaver_compositors", "weaver_compositors"), &WeaverWeb::set_weaver_compositors);
	ClassDB::bind_method(D_METHOD("get_weaver_compositors"), &WeaverWeb::get_weaver_compositors);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "weaver_compositors", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("WeaverWebCompositor")), "set_weaver_compositors", "get_weaver_compositors");
}


WeaverWeb::WeaverWeb()
{
	print_line("WEAVERWEB CONSTRUCT");
}

void WeaverWeb::set_weaver_compositors(const TypedArray<WeaverWebCompositor> &p_weaver_compositors)
{
	//Array effect_rids;
	weaver_compositors.clear();

	for (int i = 0; i < p_weaver_compositors.size(); i++)
	{
		// Cast to proper ref, if our object isn't a CompositorEffect resource this will be an empty Ref.
		Ref<WeaverWebCompositor> weaver_node = p_weaver_compositors[i];

		// We add the effect even if this is an empty Ref, this allows the UI to add new entries.
		weaver_compositors.push_back(weaver_node);

		// But we only add a rid for valid Refs
		if (weaver_node.is_valid())
		{
			//RID rid = compositor_effect->get_rid();
			//effect_rids.push_back(rid);
		}
	}
}

TypedArray<WeaverWebCompositor> WeaverWeb::get_weaver_compositors() const
{
	TypedArray<WeaverWebCompositor> arr;

	for (uint32_t i = 0; i < weaver_compositors.size(); i++)
	{
		arr.push_back(weaver_compositors[i]);
	}

	return arr;
}
