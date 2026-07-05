#pragma once

#include "scene/main/node.h"
#include "resources/weaver_spider.h"

class WeaverSpiderNest : public Node
{
	GDCLASS(WeaverSpiderNest, Node);

	//
	//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// This should not be immediatelly tied to a specific WeaverSpider... We need to make it dynamic and relly on binding indexes instead of handles to specific resournces.
	// THEREFORE, JUST EXPOSE A BINDING ID THAT THE USER CAN SET AND THE CAMERA WILL BE ASSIGNED TO A SPIDER AT RUNTIME
	//
	// AT RUNTIME, INJECT THE Compositor into the camera. Setup vanilla passes as well as WeaverPasses
	//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_spider(const Ref<WeaverSpider> &p_spider);
	Ref<WeaverSpider> get_spider() const;

	void set_slot(int slot);
	int get_slot();

private:
	int slot{ 0 }; 
	Ref<WeaverSpider> spider;
};
