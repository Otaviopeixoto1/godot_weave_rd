#pragma once

#include "core/object/object.h"
#include "modules/gdscript/gdscript.h"
#include "core/templates/rid_owner.h"

// Central manager class for all WeaverWebs .
//TODO: Define WeaverPass as the framegraph pass 
class RenderingWeaver : public Object
{
	GDCLASS(RenderingWeaver, Object);

private:
	static RenderingWeaver *singleton;

	//
	// Internal resources (todo: move to a WeaverStorage)
	//
	struct Web
	{
		Vector<RID> views;
	};
	mutable RID_Owner<Web> web_owner;

	struct View
	{
		Vector<RID> spiders;
	};
	mutable RID_Owner<View> view_owner;

	struct Spider
	{
		Vector<RID> web_nodes; 
	};
	mutable RID_Owner<Spider> spider_owner;

	struct Node
	{
		int bla;
	};
	mutable RID_Owner<Node> node_owner;


	// A script implementing the method used for hooking into the CompositorEffect _render_callback in WeaverWebNodes.
	// THIS IS A FALLBACK SOLUTION since godot doesnt expose the _render_callback to modules yet...
	Ref<GDScript> weaver_node_script;

	bool is_web(RID rid);
	bool is_view(RID rid);
	bool is_spider(RID rid);
	bool is_node(RID rid);

	void web_free(RID web);
	void view_free(RID view);
	void spider_free(RID spider);
	void node_free(RID node);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// TODO: ADD DEBUG FUNCTION TO PRINT THE HIERARCHY !! (LATER THIS WILL BE CONVERTED INTO THE WeaverDB)
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
protected:
	static void _bind_methods();
	void add(int value);

public:
	RenderingWeaver();
	static RenderingWeaver *get_singleton();

	//
	// Most of these can be simplified assuming the RenderingWeaver doesnt handle compilation and just manages the actual raw resources...
	// -> We could simplify everything to a single web resource...
	//

	RID web_create();
	void web_set_views(RID p_web, const Vector<RID> &p_views);
	Vector<RID> web_get_views(RID p_web) const;

	RID view_create();
	void view_set_spiders(RID p_view, const Vector<RID> &p_spiders);
	Vector<RID> view_get_spiders(RID p_view) const;

	RID spider_create();
	void spider_set_nodes(RID p_spider, const Vector<RID> &p_nodes);
	Vector<RID> spider_get_nodes(RID p_spider) const;

	RID node_create();

	void free_rid(RID rid);

	Ref<GDScript> get_weaver_node_script();
};
