#pragma once

#include "scene/main/node.h"
#include "resources/weaver_web.h"

class FrameWeaver : public Node
{
	GDCLASS(FrameWeaver, Node);


	// NO NEED TO SPECIFY WHICH VIEWPORT THIS SHOULD BE ATTACHED TO...
	// SINCE THE RENDERING ORDER IS PREDETERMINED, THE ORDER IN WHICH FrameWeavers are bound on the scene will determine to which WeaverView Resource they are bound
	// (index in topological order)
	// TODO: make the FrameWeaver attach to parent viewport (even the root one). Then make add warnings when multiple FrameWeavers are assigned.
	//       FINALLY INDICATE TO THE USER WHICH VIABLE INDICES THAT FrameWeaver HAS DEPENDING ON THE WEB

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_web(const Ref<WeaverWeb> &p_web);
	Ref<WeaverWeb> get_web() const;

	void set_port(int port);
	int get_port();

private:
	int port{ 0 };  
	Ref<WeaverWeb> web;
};
