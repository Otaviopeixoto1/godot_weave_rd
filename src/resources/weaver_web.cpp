#include "weaver_web.h"

#include "core/object/object.h"
#include "core/object/class_db.h"
#include "core/object/callable_mp.h"
#include "core/templates/hash_map.h"



void WeaverWeb::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_weaver_views", "weaver_views"), &WeaverWeb::set_weaver_views);
	ClassDB::bind_method(D_METHOD("get_weaver_views"), &WeaverWeb::get_weaver_views);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "weaver_views", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("WeaverView")), "set_weaver_views", "get_weaver_views");

	ClassDB::bind_method(D_METHOD("validate"), &WeaverWeb::validate);
}


void WeaverWeb::set_weaver_views(const TypedArray<WeaverView> &p_weaver_views)
{
	const Callable forward_changed = callable_mp((Resource *)this, &Resource::emit_changed);

	// Reference counted connections, so a view listed twice stays connected until its last entry is removed.
	for (const Ref<WeaverView> &weaver_view : weaver_views)
	{
		if (weaver_view.is_valid()) weaver_view->disconnect_changed(forward_changed);
	}
	weaver_views.clear();

	for (int i = 0; i < p_weaver_views.size(); i++)
	{
		Ref<WeaverView> weaver_view = p_weaver_views[i];
		weaver_views.push_back(weaver_view);

		if (weaver_view.is_valid()) weaver_view->connect_changed(forward_changed, CONNECT_REFERENCE_COUNTED);
	}

	emit_changed();
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


static void _add_issue(Array &r_issues, const String &p_severity, const String &p_message, int p_view, int p_lane)
{
	Dictionary issue;
	issue["severity"] = p_severity;
	issue["message"] = p_message;
	issue["view"] = p_view;
	issue["lane"] = p_lane;
	r_issues.push_back(issue);
}

static String _lane_name(int p_view, int p_lane)
{
	return vformat("View %d, lane %d", p_view, p_lane);
}

Array WeaverWeb::validate() const
{
	Array issues;

	HashMap<const WeaverSlot *, int> view_slot_owner; // slot -> first view using it
	HashMap<const WeaverSpider *, Vector2i> spider_owner; // spider -> first (view, lane (WeaverSpider)) using it

	for (uint32_t v = 0; v < weaver_views.size(); v++)
	{
		const Ref<WeaverView> &view = weaver_views[v];
		if (view.is_null())
		{
			_add_issue(issues, "warning", vformat("View %d is empty.", v), v, -1);
			continue;
		}

		const Ref<WeaverViewSlot> view_slot = view->get_slot();
		if (view_slot.is_null())
		{
			_add_issue(issues, "error", vformat("View %d has no slot.", v), v, -1);
		}
		else
		{
			if (const int *first = view_slot_owner.getptr(view_slot.ptr()))
			{
				_add_issue(issues, "error", vformat("View %d uses the same slot as view %d.", v, *first), v, -1);
			}
			else
			{
				view_slot_owner.insert(view_slot.ptr(), v);
			}

			if (!view_slot->is_shareable())
			{
				_add_issue(issues, "warning", vformat("View %d's slot is not saved to its own file, so scenes cannot reference it.", v), v, -1);
			}
		}

		HashMap<const WeaverSlot *, int> camera_slot_owner; // slot -> first lane (WeaverSpider) of this view using it
		const TypedArray<WeaverSpider> spiders = view->get_weaver_spiders();

		for (int l = 0; l < spiders.size(); l++)
		{
			const Ref<WeaverSpider> spider = spiders[l];
			if (spider.is_null())
			{
				_add_issue(issues, "warning", vformat("%s is empty.", _lane_name(v, l)), v, l);
				continue;
			}

			if (const Vector2i *first = spider_owner.getptr(spider.ptr()))
			{
				_add_issue(issues, "error", vformat("%s uses the same WeaverSpider as %s.", _lane_name(v, l), _lane_name(first->x, first->y)), v, l);
			}
			else
			{
				spider_owner.insert(spider.ptr(), Vector2i(v, l));
			}

			const Ref<WeaverCameraSlot> camera_slot = spider->get_slot();
			if (camera_slot.is_null())
			{
				_add_issue(issues, "error", vformat("%s has no slot.", _lane_name(v, l)), v, l);
			}
			else
			{
				if (const int *first = camera_slot_owner.getptr(camera_slot.ptr()))
				{
					_add_issue(issues, "error", vformat("%s uses the same slot as lane %d.", _lane_name(v, l), *first), v, l);
				}
				else
				{
					camera_slot_owner.insert(camera_slot.ptr(), l);
				}

				if (!camera_slot->is_shareable())
				{
					_add_issue(issues, "warning", vformat("%s's slot is not saved to its own file, so scenes cannot reference it.", _lane_name(v, l)), v, l);
				}
			}

			const TypedArray<Resource> nodes = spider->get_weaver_nodes();
			for (int n = 0; n < nodes.size(); n++)
			{
				const Ref<Resource> node = nodes[n];
				if (node.is_null())
				{
					_add_issue(issues, "warning", vformat("%s: node %d is empty.", _lane_name(v, l), n), v, l);
				}
			}
		}
	}

	return issues;
}
