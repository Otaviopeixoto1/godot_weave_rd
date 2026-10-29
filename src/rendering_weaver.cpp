#include "rendering_weaver.h"
#include "core/object/class_db.h"
#include "core/object/callable_mp.h"

#include "scene/main/node.h"
#include "scene/main/viewport.h"
#include "scene/3d/camera_3d.h"



RenderingWeaver *RenderingWeaver::singleton = nullptr;

RenderingWeaver *RenderingWeaver::get_singleton()
{
	return singleton;
}

RenderingWeaver::RenderingWeaver()
{
	ERR_FAIL_COND_MSG(singleton != nullptr, "RenderingWeaver: Singleton already exists");
	singleton = this;
}

RenderingWeaver::~RenderingWeaver()
{
	if (active_web.is_valid())
	{
		active_web->disconnect_changed(callable_mp(this, &RenderingWeaver::_queue_resolve));
	}

	if (singleton == this)
	{
		singleton = nullptr;
	}
}

void RenderingWeaver::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_active_web", "owner", "web"), &RenderingWeaver::set_active_web);
	ClassDB::bind_method(D_METHOD("clear_active_web", "owner"), &RenderingWeaver::clear_active_web);
	ClassDB::bind_method(D_METHOD("get_active_web"), &RenderingWeaver::get_active_web);

	ClassDB::bind_method(D_METHOD("register_view_nest", "nest", "viewport", "slot"), &RenderingWeaver::register_view_nest);
	ClassDB::bind_method(D_METHOD("unregister_view_nest", "nest"), &RenderingWeaver::unregister_view_nest);

	ClassDB::bind_method(D_METHOD("register_camera_nest", "nest", "camera", "slot"), &RenderingWeaver::register_camera_nest);
	ClassDB::bind_method(D_METHOD("unregister_camera_nest", "nest"), &RenderingWeaver::unregister_camera_nest);

	ClassDB::bind_method(D_METHOD("get_warnings", "nest"), &RenderingWeaver::get_warnings);
	ClassDB::bind_method(D_METHOD("get_debug_state"), &RenderingWeaver::get_debug_state);

	ADD_SIGNAL(MethodInfo("bindings_changed"));
}


//
// Registration
//
Error RenderingWeaver::set_active_web(Object *p_owner, const Ref<WeaverWeb> &p_web)
{
	ERR_FAIL_NULL_V(p_owner, ERR_INVALID_PARAMETER);

	// A freed owner never cleared its web, so another owner may take over.
	if (active_web_owner.is_valid() && active_web_owner != p_owner->get_instance_id() && ObjectDB::get_instance(active_web_owner) != nullptr)
	{
		return ERR_ALREADY_IN_USE;
	}

	const Callable queue_resolve = callable_mp(this, &RenderingWeaver::_queue_resolve);
	if (active_web.is_valid())
	{
		active_web->disconnect_changed(queue_resolve);
	}

	active_web = p_web;
	active_web_owner = p_web.is_valid() ? p_owner->get_instance_id() : ObjectID();

	// Edits anywhere in the Web (views, spiders, nodes, slots) re-run the resolution.
	if (active_web.is_valid())
	{
		active_web->connect_changed(queue_resolve);
	}

	_queue_resolve();
	return OK;
}

void RenderingWeaver::clear_active_web(Object *p_owner)
{
	ERR_FAIL_NULL(p_owner);
	if (active_web_owner != p_owner->get_instance_id())
	{
		return;
	}

	if (active_web.is_valid())
	{
		active_web->disconnect_changed(callable_mp(this, &RenderingWeaver::_queue_resolve));
	}
	active_web.unref();
	active_web_owner = ObjectID();

	_queue_resolve();
}

Ref<WeaverWeb> RenderingWeaver::get_active_web() const
{
	return active_web;
}

void RenderingWeaver::register_view_nest(Node *p_nest, Viewport *p_viewport, const Ref<WeaverViewSlot> &p_slot)
{
	ERR_FAIL_NULL(p_nest);
	ERR_FAIL_NULL(p_viewport);

	ViewBinding binding;
	binding.viewport = p_viewport->get_instance_id();
	binding.slot = p_slot;
	view_nests[p_nest->get_instance_id()] = binding;

	_queue_resolve();
}

void RenderingWeaver::unregister_view_nest(Node *p_nest)
{
	ERR_FAIL_NULL(p_nest);
	if (view_nests.erase(p_nest->get_instance_id()))
	{
		_queue_resolve();
	}
}

void RenderingWeaver::register_camera_nest(Node *p_nest, Camera3D *p_camera, const Ref<WeaverCameraSlot> &p_slot)
{
	ERR_FAIL_NULL(p_nest);
	ERR_FAIL_NULL(p_camera);

	CameraBinding binding;
	binding.camera = p_camera->get_instance_id();
	binding.slot = p_slot;
	camera_nests[p_nest->get_instance_id()] = binding;

	_queue_resolve();
}

void RenderingWeaver::unregister_camera_nest(Node *p_nest)
{
	ERR_FAIL_NULL(p_nest);
	if (camera_nests.erase(p_nest->get_instance_id()))
	{
		_queue_resolve();
	}
}


//
// Resolution
//
void RenderingWeaver::_queue_resolve()
{
	if (resolve_queued)
	{
		return;
	}
	resolve_queued = true;
	callable_mp(this, &RenderingWeaver::_flush).call_deferred();
}

void RenderingWeaver::_flush()
{
	resolve_queued = false;
	_resolve();
	emit_signal(SNAME("bindings_changed"));
}

static String _slot_name(const Ref<WeaverSlot> &p_slot)
{
	return p_slot->get_path().is_empty() ? String("<unsaved slot>") : p_slot->get_path();
}

static int _viewport_depth(const Viewport *p_viewport)
{
	int depth = 0;
	for (const Viewport *viewport = p_viewport; viewport->get_parent() != nullptr; viewport = viewport->get_parent()->get_viewport())
	{
		depth++;
	}
	return depth;
}

// Mirrors RendererViewport::_sort_active_viewports: nested viewports render first (deepest level first, across
// all branches), parents last. Within a level the engine keeps creation order; tree order stands in for it here.
static bool _renders_before(const Viewport *p_a, const Viewport *p_b)
{
	if (p_a->is_ancestor_of(p_b)) return false;
	if (p_b->is_ancestor_of(p_a)) return true;

	const int depth_a = _viewport_depth(p_a);
	const int depth_b = _viewport_depth(p_b);
	if (depth_a != depth_b) return depth_a > depth_b;

	return p_b->is_greater_than(p_a);
}

void RenderingWeaver::_resolve()
{
	// Drop nests that were freed without unregistering.
	{
		LocalVector<ObjectID> freed;
		for (const KeyValue<ObjectID, ViewBinding> &E : view_nests)
		{
			if (ObjectDB::get_instance(E.key) == nullptr) freed.push_back(E.key);
		}
		for (const ObjectID &id : freed) view_nests.erase(id);

		freed.clear();
		for (const KeyValue<ObjectID, CameraBinding> &E : camera_nests)
		{
			if (ObjectDB::get_instance(E.key) == nullptr) freed.push_back(E.key);
		}
		for (const ObjectID &id : freed) camera_nests.erase(id);
	}

	if (active_web_owner.is_valid() && ObjectDB::get_instance(active_web_owner) == nullptr)
	{
		active_web->disconnect_changed(callable_mp(this, &RenderingWeaver::_queue_resolve));
		active_web.unref();
		active_web_owner = ObjectID();
	}

	TypedArray<WeaverView> web_views;
	HashMap<const WeaverSlot *, int> view_of_slot;
	Array web_errors;
	if (active_web.is_valid())
	{
		web_views = active_web->get_weaver_views();
		for (int i = 0; i < web_views.size(); i++)
		{
			Ref<WeaverView> view = web_views[i];
			if (view.is_valid() && view->get_slot().is_valid() && !view_of_slot.has(view->get_slot().ptr()))
			{
				view_of_slot.insert(view->get_slot().ptr(), i);
			}
		}

		const Array issues = active_web->validate();
		for (int i = 0; i < issues.size(); i++)
		{
			const Dictionary issue = issues[i];
			if (String(issue["severity"]) == "error") web_errors.push_back(issue);
		}
	}

	//
	// View nests
	//
	HashMap<ObjectID, LocalVector<ObjectID>> nests_in_viewport;
	for (KeyValue<ObjectID, ViewBinding> &E : view_nests)
	{
		ViewBinding &binding = E.value;
		binding.view = -1;
		binding.reason = String();
		binding.warnings.clear();

		if (Object::cast_to<Viewport>(ObjectDB::get_instance(binding.viewport)) == nullptr)
		{
			binding.reason = "The viewport was freed.";
			continue;
		}
		nests_in_viewport[binding.viewport].push_back(E.key);

		if (binding.slot.is_null())
		{
			binding.reason = "No view slot is set.";
			binding.warnings.push_back(binding.reason);
			continue;
		}
		if (active_web.is_null())
		{
			binding.reason = "No WeaverWeb is active.";
			continue;
		}

		const int *view = view_of_slot.getptr(binding.slot.ptr());
		if (view == nullptr)
		{
			binding.reason = vformat("The view slot %s is not in the active WeaverWeb.", _slot_name(binding.slot));
			binding.warnings.push_back(binding.reason);
			continue;
		}
		binding.view = *view;
	}

	// One viewport can only run one view.
	HashMap<ObjectID, ObjectID> nest_of_viewport;
	for (const KeyValue<ObjectID, LocalVector<ObjectID>> &E : nests_in_viewport)
	{
		if (E.value.size() == 1)
		{
			nest_of_viewport.insert(E.key, E.value[0]);
			continue;
		}
		for (const ObjectID &id : E.value)
		{
			ViewBinding &binding = view_nests[id];
			binding.view = -1;
			binding.reason = "Its viewport has more than one view nest.";
			binding.warnings.push_back(vformat("%d view nests are bound to the same viewport. Only one is allowed.", E.value.size()));
		}
	}

	// One view slot bound to several viewports, and viewports rendering in a different order than the Web declares.
	{
		LocalVector<ObjectID> resolved;
		for (const KeyValue<ObjectID, ViewBinding> &E : view_nests)
		{
			if (E.value.view >= 0) resolved.push_back(E.key);
		}

		for (const ObjectID &id_a : resolved)
		{
			ViewBinding &a = view_nests[id_a];
			const Viewport *viewport_a = Object::cast_to<Viewport>(ObjectDB::get_instance(a.viewport));

			for (const ObjectID &id_b : resolved)
			{
				if (id_a == id_b) continue;
				const ViewBinding &b = view_nests[id_b];
				const Viewport *viewport_b = Object::cast_to<Viewport>(ObjectDB::get_instance(b.viewport));

				if (a.view == b.view)
				{
					a.warnings.push_back(vformat("The view slot %s is also bound to the viewport %s.", _slot_name(a.slot), viewport_b->get_name()));
				}
				else if (viewport_a->is_inside_tree() && viewport_b->is_inside_tree())
				{
					// Report on both nests of the pair.
					const bool a_first_in_web = a.view < b.view;
					const bool a_renders_first = _renders_before(viewport_a, viewport_b);
					if (a_first_in_web != a_renders_first)
					{
						a.warnings.push_back(vformat("Render order: the Web declares view %d %s view %d, but viewport %s renders %s viewport %s.",
								a.view, a_first_in_web ? "before" : "after", b.view,
								viewport_a->get_name(), a_renders_first ? "before" : "after", viewport_b->get_name()));
					}
				}
			}
		}
	}

	//
	// Camera nests
	//
	HashMap<ObjectID, LocalVector<ObjectID>> nests_on_camera;
	for (KeyValue<ObjectID, CameraBinding> &E : camera_nests)
	{
		CameraBinding &binding = E.value;
		binding.view = -1;
		binding.lane = -1;
		binding.reason = String();
		binding.warnings.clear();

		Camera3D *camera = Object::cast_to<Camera3D>(ObjectDB::get_instance(binding.camera));
		if (camera == nullptr)
		{
			binding.reason = "The camera was freed.";
			continue;
		}
		nests_on_camera[binding.camera].push_back(E.key);

		if (binding.slot.is_null())
		{
			binding.reason = "No camera slot is set.";
			binding.warnings.push_back(binding.reason);
			continue;
		}
		if (camera->get_compositor().is_valid())
		{
			binding.warnings.push_back("The camera has its own Compositor. It is overridden while the camera is bound.");
		}
		if (active_web.is_null())
		{
			binding.reason = "No WeaverWeb is active.";
			continue;
		}
		if (!camera->is_inside_tree())
		{
			binding.reason = "The camera is not in the scene tree.";
			continue;
		}

		const ObjectID *view_nest = nest_of_viewport.getptr(camera->get_viewport()->get_instance_id());
		if (view_nest == nullptr)
		{
			binding.reason = vformat("No view nest is bound to the camera's viewport (%s).", camera->get_viewport()->get_name());
			continue;
		}
		const ViewBinding &view_binding = view_nests[*view_nest];
		if (view_binding.view < 0)
		{
			binding.reason = "The view nest of the camera's viewport is pending: " + view_binding.reason;
			continue;
		}

		const Ref<WeaverView> view = web_views[view_binding.view];
		const TypedArray<WeaverSpider> spiders = view->get_weaver_spiders();
		for (int i = 0; i < spiders.size(); i++)
		{
			const Ref<WeaverSpider> spider = spiders[i];
			if (spider.is_valid() && spider->get_slot() == binding.slot)
			{
				binding.lane = i;
				break;
			}
		}
		if (binding.lane < 0)
		{
			binding.reason = vformat("The camera slot %s is not in view %d of the active WeaverWeb.", _slot_name(binding.slot), view_binding.view);
			binding.warnings.push_back(binding.reason);
			continue;
		}
		binding.view = view_binding.view;
	}

	// One camera can only run one lane (it has a single compositor).
	for (const KeyValue<ObjectID, LocalVector<ObjectID>> &E : nests_on_camera)
	{
		if (E.value.size() == 1) continue;
		for (const ObjectID &id : E.value)
		{
			CameraBinding &binding = camera_nests[id];
			binding.view = -1;
			binding.lane = -1;
			binding.reason = "Its camera has more than one camera nest.";
			binding.warnings.push_back(vformat("%d camera nests are bound to the same camera. Only one is allowed.", E.value.size()));
		}
	}

	//
	// Web errors: reported on the nests bound to the affected view/lane, or on every nest when none is.
	//
	for (int i = 0; i < web_errors.size(); i++)
	{
		const Dictionary issue = web_errors[i];
		const int view = issue["view"];
		const int lane = issue["lane"];
		const String message = "Active WeaverWeb: " + String(issue["message"]);

		// Most specific first: camera nests of the lane, then nests of the view, then everyone.
		bool reported = false;
		if (view >= 0)
		{
			for (KeyValue<ObjectID, CameraBinding> &E : camera_nests)
			{
				if (E.value.view == view && (lane < 0 || E.value.lane == lane)) { E.value.warnings.push_back(message); reported = true; }
			}
			if (lane < 0 || !reported)
			{
				for (KeyValue<ObjectID, ViewBinding> &E : view_nests)
				{
					if (E.value.view == view) { E.value.warnings.push_back(message); reported = true; }
				}
			}
		}
		if (!reported)
		{
			for (KeyValue<ObjectID, ViewBinding> &E : view_nests) E.value.warnings.push_back(message);
			for (KeyValue<ObjectID, CameraBinding> &E : camera_nests) E.value.warnings.push_back(message);
		}
	}
}


//
// Queries
//
PackedStringArray RenderingWeaver::get_warnings(Node *p_nest) const
{
	ERR_FAIL_NULL_V(p_nest, PackedStringArray());

	if (const ViewBinding *binding = view_nests.getptr(p_nest->get_instance_id()))
	{
		return binding->warnings;
	}
	if (const CameraBinding *binding = camera_nests.getptr(p_nest->get_instance_id()))
	{
		return binding->warnings;
	}
	return PackedStringArray();
}

static String _nest_name(ObjectID p_nest)
{
	const Node *nest = Object::cast_to<Node>(ObjectDB::get_instance(p_nest));
	if (nest == nullptr) return String();
	return nest->is_inside_tree() ? String(nest->get_path()) : String(nest->get_name());
}

namespace {
struct DebugEntry
{
	String key;
	Dictionary state;
	bool operator<(const DebugEntry &p_other) const { return key < p_other.key; }
};
}

Array RenderingWeaver::get_debug_state() const
{
	LocalVector<DebugEntry> entries;

	// Nests freed without unregistering are dropped on the next resolve; skip them until then.
	for (const KeyValue<ObjectID, ViewBinding> &E : view_nests)
	{
		if (ObjectDB::get_instance(E.key) == nullptr) continue;

		Dictionary d;
		d["kind"] = "view";
		d["nest"] = _nest_name(E.key);
		d["slot"] = E.value.slot.is_valid() ? _slot_name(E.value.slot) : String();
		d["state"] = E.value.view >= 0 ? "resolved" : "pending";
		d["reason"] = E.value.reason;
		d["view"] = E.value.view;
		d["lane"] = -1;
		entries.push_back({ "view:" + String(d["nest"]), d });
	}

	for (const KeyValue<ObjectID, CameraBinding> &E : camera_nests)
	{
		if (ObjectDB::get_instance(E.key) == nullptr) continue;

		Dictionary d;
		d["kind"] = "camera";
		d["nest"] = _nest_name(E.key);
		d["slot"] = E.value.slot.is_valid() ? _slot_name(E.value.slot) : String();
		d["state"] = E.value.lane >= 0 ? "resolved" : "pending";
		d["reason"] = E.value.reason;
		d["view"] = E.value.view;
		d["lane"] = E.value.lane;
		entries.push_back({ "camera:" + String(d["nest"]), d });
	}

	// Sorted so the result does not depend on registration order.
	entries.sort();

	Array result;
	for (const DebugEntry &entry : entries)
	{
		result.push_back(entry.state);
	}
	return result;
}
