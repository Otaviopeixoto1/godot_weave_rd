#pragma once

#include "scene/main/node.h"
#include "resources/weaver_web.h"

class FrameWeaver : public Node
{
	GDCLASS(FrameWeaver, Node);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_web(const Ref<WeaverWeb> &p_web);
	Ref<WeaverWeb> get_web() const;

private:
	//Hold and register the web into the RenderingWeaver... 
	Ref<WeaverWeb> web;
};
