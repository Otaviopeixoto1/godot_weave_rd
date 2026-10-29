#pragma once

#include "core/object/object.h"
#include "core/templates/hash_map.h"
#include "core/variant/typed_array.h"

#include "resources/weaver_web.h"
#include "resources/weaver_slot.h"

class Node;
class Viewport;
class Camera3D;
class WeaverSpiderInstance;

//
// RenderingWeaver is the singleton representing the binding registry between WeaverWebs and the scene.
// It stores the active Web and the anchors and nests registered from scenes and resolves which camera runs which lane (WeaverSpider).
// Registration order does not matter: an entry stays pending (with a reason) until the missing piece is registered.
//
// Everything is stored by ObjectID, so nodes freed without unregistering are simply dropped on the next resolve.
// _resolve() is queued to be called at most once per frame, and emits the bindings_changed signal once per batch of changes.
//
class RenderingWeaver : public Object
{
	GDCLASS(RenderingWeaver, Object);

public:
	struct ViewBinding
	{
		ObjectID viewport;
		Ref<WeaverViewSlot> slot;

		int view = -1;
		String reason; // Reason why its not resolved. Empty when resolved.
		PackedStringArray warnings;
	};

	struct CameraBinding
	{
		ObjectID camera;
		Ref<WeaverCameraSlot> slot;

		int view = -1;
		int lane = -1;
		String reason; // Reason why its not resolved. Empty when resolved.
		PackedStringArray warnings;
	};

	RenderingWeaver();
	virtual ~RenderingWeaver();
	static RenderingWeaver *get_singleton();

	// Sets the currently active WeaverWeb. Only one owner (a WeaverSpinner) can hold the active Web. Returns ERR_ALREADY_IN_USE if another owner holds it.
	Error set_active_web(Object *p_owner, const Ref<WeaverWeb> &p_web);
	void clear_active_web(Object *p_owner);
	Ref<WeaverWeb> get_active_web() const;
	ObjectID get_active_web_owner() const { return active_web_owner; }

	void register_view_anchor(Node *p_anchor, Viewport *p_viewport, const Ref<WeaverViewSlot> &p_slot);
	void unregister_view_anchor(Node *p_anchor);

	void register_camera_nest(Node *p_nest, Camera3D *p_camera, const Ref<WeaverCameraSlot> &p_slot);
	void unregister_camera_nest(Node *p_nest);

	PackedStringArray get_warnings(const Node *p_nest) const;
	Array get_debug_state() const;

	const HashMap<ObjectID, CameraBinding> &get_camera_bindings() const { return camera_nests; }

protected:
	static void _bind_methods();

private:
	static RenderingWeaver *singleton;

	ObjectID active_web_owner;
	Ref<WeaverWeb> active_web;

	HashMap<ObjectID, ViewBinding> view_anchors;
	HashMap<ObjectID, CameraBinding> camera_nests;

	// Runtime compositor of each registered/resolved camera.
	HashMap<ObjectID, WeaverSpiderInstance *> spider_instances;

	bool resolve_queued = false;

	void _queue_resolve();
	void _flush();
	void _resolve();


	void _sync_spider_instances(const TypedArray<WeaverView> &p_web_views);
	void _free_spider_instance(const ObjectID &p_nest);
};
