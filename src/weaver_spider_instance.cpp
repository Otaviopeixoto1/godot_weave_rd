#include "weaver_spider_instance.h"

#include "core/object/callable_mp.h"
#include "scene/3d/camera_3d.h"
#include "scene/resources/compositor.h"
#include "servers/rendering/rendering_server.h"


WeaverSpiderInstance::WeaverSpiderInstance(Camera3D *p_camera, const Ref<WeaverSpider> &p_spider)
{
	camera = p_camera->get_instance_id();
	camera_rid = p_camera->get_camera();
	spider = p_spider;
	_build();
}

WeaverSpiderInstance::~WeaverSpiderInstance()
{
	_teardown();
}

// Runs on the render thread. The node copies are released when the bound array is, after their cleanup.
static void _cleanup_nodes(const Array &p_instances)
{
	for (int i = 0; i < p_instances.size(); i++)
	{
		Ref<WeaverWebNode> instance = p_instances[i];
		instance->cleanup();
	}
}

void WeaverSpiderInstance::_build()
{
	RenderingServer *rs = RenderingServer::get_singleton();
	ERR_FAIL_NULL(rs);

	TypedArray<RID> effect_rids;
	const TypedArray<Resource> nodes = spider->get_weaver_nodes();
	for (int i = 0; i < nodes.size(); i++)
	{
		const Ref<Resource> node = nodes[i];
		if (node.is_null()) { continue; }

		if (const Ref<CompositorEffect> effect = node; effect.is_valid())
		{
			entries.push_back({ Ref<WeaverWebNode>(), effect->get_rid() });
			effect_rids.push_back(effect->get_rid());
			continue;
		}

		const Ref<WeaverWebNode> description = node;
		if (description.is_null()) { continue; }

		// Per-camera copy: keeps the user's script, state set in setup() is never shared between cameras.
		Entry entry;
		entry.instance = description->duplicate();
		entry.effect = rs->compositor_effect_create();
		rs->call_on_render_thread(callable_mp(entry.instance.ptr(), &WeaverWebNode::setup));

		rs->compositor_effect_set_enabled(entry.effect, entry.instance->get_enabled());
		rs->compositor_effect_set_flag(entry.effect, RSE::COMPOSITOR_EFFECT_FLAG_ACCESS_RESOLVED_COLOR, entry.instance->get_access_resolved_color());
		rs->compositor_effect_set_flag(entry.effect, RSE::COMPOSITOR_EFFECT_FLAG_ACCESS_RESOLVED_DEPTH, entry.instance->get_access_resolved_depth());
		rs->compositor_effect_set_flag(entry.effect, RSE::COMPOSITOR_EFFECT_FLAG_NEEDS_MOTION_VECTORS, entry.instance->get_needs_motion_vectors());
		rs->compositor_effect_set_flag(entry.effect, RSE::COMPOSITOR_EFFECT_FLAG_NEEDS_ROUGHNESS, entry.instance->get_needs_normal_roughness());
		rs->compositor_effect_set_flag(entry.effect, RSE::COMPOSITOR_EFFECT_FLAG_NEEDS_SEPARATE_SPECULAR, entry.instance->get_needs_separate_specular());
		rs->compositor_effect_set_callback(entry.effect, RSE::CompositorEffectCallbackType(entry.instance->get_stage()), callable_mp(entry.instance.ptr(), &WeaverWebNode::render));

		effect_rids.push_back(entry.effect);
		entries.push_back(entry);
	}

	compositor = rs->compositor_create();
	rs->compositor_set_compositor_effects(compositor, effect_rids);
	rs->camera_set_compositor(camera_rid, compositor);
}

void WeaverSpiderInstance::_teardown()
{
	RenderingServer *rs = RenderingServer::get_singleton();
	if (rs == nullptr)
	{
		// The rendering server is already gone (shutdown): its RIDs went with it.
		entries.clear();
		return;
	}

	// Give the camera its own compositor back (only if the camera still exists, its RID dies with it).
	if (const Camera3D *cam = Object::cast_to<Camera3D>(ObjectDB::get_instance(camera)))
	{
		const Ref<Compositor> own = cam->get_compositor();
		rs->camera_set_compositor(camera_rid, own.is_valid() ? own->get_rid() : RID());
	}

	if (compositor.is_valid())
	{
		rs->free_rid(compositor);
		compositor = RID();
	}

	Array instances;
	for (const Entry &entry : entries)
	{
		if (entry.instance.is_null()) { continue; } // Plain CompositorEffect: not ours to free.
		rs->free_rid(entry.effect);
		instances.push_back(entry.instance);
	}
	entries.clear();

	// Queued after the frees, so no callback can run on a node after its cleanup.
	if (!instances.is_empty())
	{
		rs->call_on_render_thread(callable_mp_static(&_cleanup_nodes).bind(instances));
	}
}
