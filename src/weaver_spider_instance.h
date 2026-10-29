#pragma once

#include "core/object/object_id.h"
#include "core/templates/local_vector.h"
#include "core/templates/rid.h"

#include "resources/weaver_spider.h"
#include "resources/weaver_web_node.h"

class Camera3D;

//
// Runtime compositor of one resolved camera nest (internal, owned by RenderingWeaver).
// Built straight on the RenderingServer: no Compositor / CompositorEffect objects are created and camera.compositor is
// never written, so nothing ends up in scene files.
//
// - WeaverWebNode entries are duplicated per camera (per-camera state) and get an effect RID created here.
// - Plain CompositorEffect entries are used as they are (their RID is not owned here).
// setup() and cleanup() of the node copies run on the render thread, like render().
//
class WeaverSpiderInstance
{
public:
	WeaverSpiderInstance(Camera3D *p_camera, const Ref<WeaverSpider> &p_spider);
	~WeaverSpiderInstance();

	ObjectID get_camera() const { return camera; }
	const Ref<WeaverSpider> &get_spider() const { return spider; }

private:
	struct Entry
	{
		Ref<WeaverWebNode> instance; // null for a plain CompositorEffect
		RID effect;
	};

	ObjectID camera;
	RID camera_rid;
	Ref<WeaverSpider> spider;

	RID compositor;
	LocalVector<Entry> entries;

	void _build();
	void _teardown();
};
