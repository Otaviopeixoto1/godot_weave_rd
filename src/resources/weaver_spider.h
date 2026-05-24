#pragma once

#include "scene/resources/compositor.h"
#include "weaver_web_node.h"

class WeaverSpider : public Resource 
{
	GDCLASS(WeaverSpider, Resource);

protected:
	static void _bind_methods();
public:
	WeaverSpider();
	virtual ~WeaverSpider();

	void set_view(RID p_view);
	virtual RID get_rid() const override { return spider; }

	// TODO: ADD INPUT FOR vanilla CompositorEffects as well
	void set_weaver_nodes(const TypedArray<WeaverWebNode> &p_weaver_nodes);
	TypedArray<WeaverWebNode> get_weaver_nodes() const;

private:
	RID spider;
	RID view;
	LocalVector<Ref<WeaverWebNode>> weaver_nodes;
};
