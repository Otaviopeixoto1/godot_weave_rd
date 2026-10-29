#pragma once

#include "scene/resources/compositor.h"
#include "core/io/resource.h"
#include "core/object/gdvirtual.gen.h"
#include "servers/rendering/storage/render_data.h"


//
// WeaverWebNode A pass inside a Stage in a WeaverSpider. This is only a description the actual CompositorEffect
// is created directly through the RenderingServer.
//
// Per-camera state lives in a duplicate() of the node made by the runtime. This is so that members set inside setup() are never
// shared, each scene has its own copy.
//
// IMPORTANT: render(), setup() and cleanup() all still run on the RENDER THREAD. Don't touch the scene tree !
// (The inspector properties are written on the main thread)
//
// Overriding, from the most derived implementation down:
// - GDScript: override _render, _setup, _cleanup
// - C++: override the protected _render_native, _setup_native, _cleanup_native.
// When a script or extension overrides a hook, the native implementation is NOT CALLED for that hook.
//
class WeaverWebNode : public Resource
{
	GDCLASS(WeaverWebNode, Resource);

protected:

	static void _bind_methods();

	//
	// Native override hooks
	//
	virtual void _render_native(int p_stage, const RenderData *p_render_data) {}
	virtual void _setup_native() {}
	virtual void _cleanup_native() {}

	GDVIRTUAL2(_render, int, const RenderData *)
	GDVIRTUAL0(_setup)
	GDVIRTUAL0(_cleanup)

public:
	enum Stage {
		PRE_OPAQUE = CompositorEffect::EFFECT_CALLBACK_TYPE_PRE_OPAQUE,
		POST_OPAQUE = CompositorEffect::EFFECT_CALLBACK_TYPE_POST_OPAQUE,
		POST_SKY = CompositorEffect::EFFECT_CALLBACK_TYPE_POST_SKY,
		PRE_TRANSPARENT = CompositorEffect::EFFECT_CALLBACK_TYPE_PRE_TRANSPARENT,
		POST_TRANSPARENT = CompositorEffect::EFFECT_CALLBACK_TYPE_POST_TRANSPARENT,
		MAX = CompositorEffect::EFFECT_CALLBACK_TYPE_MAX,
	};

	WeaverWebNode() = default;
	virtual ~WeaverWebNode() = default;

	void set_enabled(bool p_enabled);
	bool get_enabled() const;

	void set_stage(Stage p_stage);
	Stage get_stage() const;

	void set_access_resolved_color(bool p_enabled);
	bool get_access_resolved_color() const;

	void set_access_resolved_depth(bool p_enabled);
	bool get_access_resolved_depth() const;

	void set_needs_motion_vectors(bool p_enabled);
	bool get_needs_motion_vectors() const;

	void set_needs_normal_roughness(bool p_enabled);
	bool get_needs_normal_roughness() const;

	void set_needs_separate_specular(bool p_enabled);
	bool get_needs_separate_specular() const;

	// Entry points used by the runtime (render thread). Not virtual: override the hooks above instead.
	void render(int p_stage, const RenderData *p_render_data);
	void setup();
	void cleanup();

private:
	bool enabled = true;
	bool access_resolved_color = false;
	bool access_resolved_depth = false;
	bool needs_motion_vectors = false;
	bool needs_normal_roughness = false;
	bool needs_separate_specular = false;

	Stage stage = POST_TRANSPARENT;
};

VARIANT_ENUM_CAST(WeaverWebNode::Stage);
