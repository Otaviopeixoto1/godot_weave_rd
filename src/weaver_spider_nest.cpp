#include "weaver_spider_nest.h"



void WeaverSpiderNest::_notification(int p_what)
{
	switch (p_what)
	{
		case Node::NOTIFICATION_ENTER_TREE: {
			print_line("WeaverSpiderNest ENTER TRIGGERED");
		} break;
		case Node::NOTIFICATION_READY: {
			print_line("WeaverSpiderNest READY TRIGGERED");
		} break;
		case Node::NOTIFICATION_EXIT_TREE: {
			print_line("WeaverSpiderNest EXIT TRIGGERED");
		} break;

		// We can use NOTIFICATION_PARENTED to identify changes to the tree and issue warnings !
	}
}

void WeaverSpiderNest::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_spider", "spider"), &WeaverSpiderNest::set_spider);
	ClassDB::bind_method(D_METHOD("get_spider"), &WeaverSpiderNest::get_spider);
	//ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "spider", PROPERTY_HINT_RESOURCE_TYPE, "WeaverSpider"), "set_spider", "get_spider");

	ClassDB::bind_method(D_METHOD("set_slot", "slot"), &WeaverSpiderNest::set_slot);
	ClassDB::bind_method(D_METHOD("get_slot"), &WeaverSpiderNest::get_slot);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "slot"), "set_slot", "get_slot");
}

void WeaverSpiderNest::set_spider(const Ref<WeaverSpider> &p_spider)
{
	if (spider == p_spider) { return; }
	//Do some setup with the new WeaverSpider...
	spider = p_spider;
}

Ref<WeaverSpider> WeaverSpiderNest::get_spider() const
{
	return spider;
}

void WeaverSpiderNest::set_slot(int slot)
{
	this->slot = slot;
}

int WeaverSpiderNest::get_slot()
{
	return slot;
}
