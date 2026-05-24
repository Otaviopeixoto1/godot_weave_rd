#include "rendering_weaver.h"
#include "core/object/class_db.h"



RenderingWeaver *RenderingWeaver::singleton = nullptr;

RenderingWeaver *RenderingWeaver::get_singleton()
{
	return singleton;
}

//
// Webs
//
RID RenderingWeaver::web_create()
{	
	RID rid = web_owner.allocate_rid();
	web_owner.initialize_rid(rid, Web());
	return rid;
}

void RenderingWeaver::web_set_views(RID p_web, const Vector<RID> &p_views)
{
	Web *web = web_owner.get_or_null(p_web);
	ERR_FAIL_NULL(web);

	web->views.clear();
	for (const RID &view : p_views)
	{
		if (is_view(view))
		{
			web->views.push_back(view);
		}
	}
}

Vector<RID> RenderingWeaver::web_get_views(RID p_web) const
{
	Web *web = web_owner.get_or_null(p_web);
	ERR_FAIL_NULL_V(web, Vector<RID>());
	return web->views;
}

//
// Views
//
RID RenderingWeaver::view_create()
{
	RID rid = view_owner.allocate_rid();
	view_owner.initialize_rid(rid, View());
	return rid;
}

void RenderingWeaver::view_set_spiders(RID p_view, const Vector<RID> &p_spiders)
{
	View *view = view_owner.get_or_null(p_view);
	ERR_FAIL_NULL(view);

	view->spiders.clear();
	for (const RID &spider : p_spiders)
	{
		if (is_spider(spider))
		{
			view->spiders.push_back(spider);
		}
	}
}

Vector<RID> RenderingWeaver::view_get_spiders(RID p_view) const
{
	View *view = view_owner.get_or_null(p_view);
	ERR_FAIL_NULL_V(view, Vector<RID>());
	return view->spiders;
}

//
// Spiders
//
RID RenderingWeaver::spider_create()
{
	RID rid = spider_owner.allocate_rid();
	spider_owner.initialize_rid(rid, Spider());
	return rid;
}

void RenderingWeaver::spider_set_nodes(RID p_spider, const Vector<RID> &p_nodes)
{
	Spider *spider = spider_owner.get_or_null(p_spider);
	ERR_FAIL_NULL(spider);

	spider->web_nodes.clear();
	for (const RID &node : p_nodes)
	{
		if (is_node(node))
		{
			spider->web_nodes.push_back(node);
		}
	}
}

Vector<RID> RenderingWeaver::spider_get_nodes(RID p_spider) const
{
	Spider *spider = spider_owner.get_or_null(p_spider);
	ERR_FAIL_NULL_V(spider, Vector<RID>());
	return spider->web_nodes;
}

//
// Nodes
//
RID RenderingWeaver::node_create()
{
	RID rid = node_owner.allocate_rid();
	node_owner.initialize_rid(rid, Node());
	return rid;
}

void RenderingWeaver::free_rid(RID rid)
{
	if (is_web(rid))
	{
		web_free(rid);
	}
	else if (is_view(rid))
	{
		view_free(rid);
	}
	else if (is_spider(rid))
	{
		spider_free(rid);
	}
}

bool RenderingWeaver::is_web(RID rid)
{
	return web_owner.owns(rid);
}

bool RenderingWeaver::is_view(RID rid)
{
	return view_owner.owns(rid);
}

bool RenderingWeaver::is_spider(RID rid)
{
	return spider_owner.owns(rid);
}

bool RenderingWeaver::is_node(RID rid)
{
	return false;
}

void RenderingWeaver::web_free(RID web)
{
	web_owner.free(web);
}

void RenderingWeaver::view_free(RID view)
{
	View *view_ptr = view_owner.get_or_null(view);
	ERR_FAIL_NULL(view_ptr);

	// Remove this RID from any web that uses it.
	for (const RID &web_rid : web_owner.get_owned_list())
	{
		Web *web = web_owner.get_or_null(web_rid);
		if (web)
		{
			web->views.erase(view);
		}
	}

	view_owner.free(view);
}

void RenderingWeaver::spider_free(RID spider)
{
	Spider *spider_ptr = spider_owner.get_or_null(spider);
	ERR_FAIL_NULL(spider_ptr);

	// Remove this RID from any web that uses it.
	for (const RID &view_rid : view_owner.get_owned_list())
	{
		View *web = view_owner.get_or_null(view_rid);
		if (web)
		{
			web->spiders.erase(spider);
		}
	}

	spider_owner.free(spider);
}

void RenderingWeaver::node_free(RID node)
{
	Node *node_ptr = node_owner.get_or_null(node);
	ERR_FAIL_NULL(node_ptr);

	// Remove this RID from any web that uses it.
	for (const RID &spider_rid : spider_owner.get_owned_list())
	{
		Spider *spider = spider_owner.get_or_null(spider_rid);
		if (spider)
		{
			spider->web_nodes.erase(node);
		}
	}

	node_owner.free(node);
}

void RenderingWeaver::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("add", "value"), &RenderingWeaver::add);
	//This works because its a simple method that we are binding. It might be necessary to use a dummy class with _bind_methods() defined just to bind the correct singleton methods
	//-> This problem is created because ClassDB::bind_method might not use the singleton instance for binding the function pointer but in a dummy class
	//   we can call the static get_singleton(), which solver it (?)
}

void RenderingWeaver::add(int value)
{
	print_line(value);
}

RenderingWeaver::RenderingWeaver()
{
	ERR_FAIL_COND_MSG(singleton != nullptr, "RenderingWeaver: Singleton already exists");
	singleton = this;
}



Ref<GDScript> RenderingWeaver::get_weaver_node_script()
{
	if (!weaver_node_script.is_valid())
	{
		weaver_node_script.instantiate();
		weaver_node_script->set_source_code(
				"@tool\n"
				"extends WeaverWebNode\n"
				"func _render_callback(p_effect_callback_type, p_render_data):\n"
				"   _internal_weaver_render_callback(p_effect_callback_type, p_render_data)\n");
		Error err = weaver_node_script->reload();
		ERR_FAIL_COND_V_MSG(err != OK, nullptr, "FAILED COMPILING SCRIPT");
	}

	return weaver_node_script;
}
