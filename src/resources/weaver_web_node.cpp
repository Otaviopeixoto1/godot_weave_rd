#include "weaver_web_node.h"

#include "../rendering_weaver.h"



void WeaverWebNode::_notification(int p_what)
{
	switch (p_what) {
		case NOTIFICATION_POSTINITIALIZE: {
			callable_mp(this, &WeaverWebNode::_attach_script).call_deferred();
		} break;
		case NOTIFICATION_PREDELETE: {
			//Delete script ??
		} break;
	}
}

void WeaverWebNode::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("_internal_weaver_render_callback", "effect_callback_type", "render_data"), &WeaverWebNode::_internal_weaver_render_callback);
}

void WeaverWebNode::_internal_weaver_render_callback(int p_effect_callback_type, const RenderData *p_render_data)
{
	print_line("_internal_weaver_render_callback");
}

WeaverWebNode::WeaverWebNode()
{
	RenderingWeaver *weaver = RenderingWeaver::get_singleton();
	if (weaver != nullptr)
	{
		node = weaver->node_create();
	}
}

WeaverWebNode::~WeaverWebNode()
{
	RenderingWeaver *weaver = RenderingWeaver::get_singleton();
	if (weaver != nullptr && node.is_valid())
	{
		weaver->free_rid(node);
	}
}

void WeaverWebNode::set_spider(RID p_spider)
{
	//TODO: trigger the signal for the node containing this WeaverSpider resource to signal to RenderingWeaver that its active
	//   ----> MAYBE NOT NECESSARY SINCE WE CAN USE THAT ALL RESOURCES INITIALIZE BEFORE THE SCENE (???)
	spider = p_spider;
}

void WeaverWebNode::_attach_script()
{
	//TODO: MANAGE THESE SCRIPTS FROM RenderingWeaver
	Ref<GDScript> script = RenderingWeaver::get_singleton()->get_weaver_node_script();
	if (script.is_valid()) set_script(script);

	// ALSO DETATCH IT !!! OTHERWISE THIS WOULD TRY TO GET SERIALIZED TO THE SCENE !
}
