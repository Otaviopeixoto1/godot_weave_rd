#pragma once 

#include "core/io/resource.h"
#include "weaver_spider.h"

class WeaverView : public Resource
{
	GDCLASS(WeaverView, Resource);

protected:
	static void _bind_methods();

public:
	WeaverView();
	virtual ~WeaverView();

	void set_web(RID p_web);
	virtual RID get_rid() const override { return view; }

	void set_weaver_spiders(const TypedArray<WeaverSpider> &p_weaver_spiders);
	TypedArray<WeaverSpider> get_weaver_spiders() const;

private:
	RID view;
	RID web;
	LocalVector<Ref<WeaverSpider>> weaver_spiders;
};
