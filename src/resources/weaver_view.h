#pragma once 

#include "core/io/resource.h"
#include "weaver_spider.h"
#include "weaver_slot.h"

#include "core/variant/typed_array.h"
#include "core/object/object.h"
#include "core/object/class_db.h"

class WeaverView : public Resource
{
	GDCLASS(WeaverView, Resource);

protected:
	static void _bind_methods();

public:
	WeaverView() = default;
	virtual ~WeaverView() = default;

	void set_slot(const Ref<WeaverViewSlot> &p_slot);
	Ref<WeaverViewSlot> get_slot() const;

	void set_weaver_spiders(const TypedArray<WeaverSpider> &p_weaver_spiders);
	TypedArray<WeaverSpider> get_weaver_spiders() const;

private:
	Ref<WeaverViewSlot> slot;
	LocalVector<Ref<WeaverSpider>> weaver_spiders;
};
