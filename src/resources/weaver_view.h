#pragma once 

#include "core/io/resource.h"
#include "weaver_web_compositor.h"

class WeaverView : public Resource
{
	GDCLASS(WeaverView, Resource);

protected:
	static void _bind_methods();

public:
	void set_weaver_compositors(const TypedArray<WeaverWebCompositor> &p_weaver_compositors);
	TypedArray<WeaverWebCompositor> get_weaver_compositors() const;

private:
	//Ref<WeaverWeb> web; //---> HERE INSTEAD OF WEB REFERENCE, STORE A RID of WEB
	LocalVector<Ref<WeaverWebCompositor>> weaver_compositors;
};
