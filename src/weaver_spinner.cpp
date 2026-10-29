#include "weaver_spinner.h"
#include "rendering_weaver.h"

#include "core/object/class_db.h"
#include "core/object/callable_mp.h"


void WeaverSpinner::_notification(int p_what)
{
	RenderingWeaver *weaver = RenderingWeaver::get_singleton();
	if (weaver == nullptr) return;

	switch (p_what) {
		case Node::NOTIFICATION_ENTER_TREE: {
			weaver->connect(SNAME("bindings_changed"), callable_mp(this, &WeaverSpinner::_on_bindings_changed));
			_activate();
		} break;
		case Node::NOTIFICATION_EXIT_TREE: {
			weaver->disconnect(SNAME("bindings_changed"), callable_mp(this, &WeaverSpinner::_on_bindings_changed));
			weaver->clear_active_web(this);
		} break;
	}
}

void WeaverSpinner::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_web", "web"), &WeaverSpinner::set_web);
	ClassDB::bind_method(D_METHOD("get_web"), &WeaverSpinner::get_web);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "web", PROPERTY_HINT_RESOURCE_TYPE, "WeaverWeb"), "set_web", "get_web");
}

void WeaverSpinner::set_web(const Ref<WeaverWeb> &p_web)
{
	if (web == p_web) { return; }
	web = p_web;

	if (is_inside_tree())
	{
		_activate();
		update_configuration_warnings();
	}
}

Ref<WeaverWeb> WeaverSpinner::get_web() const
{
	return web;
}

void WeaverSpinner::_activate()
{
	RenderingWeaver *weaver = RenderingWeaver::get_singleton();
	if (web.is_valid())
	{
		// Fails with ERR_ALREADY_IN_USE while another spinner is active; reported as a configuration warning.
		weaver->set_active_web(this, web);
	}
	else
	{
		weaver->clear_active_web(this);
	}
	update_configuration_warnings();
}

void WeaverSpinner::_on_bindings_changed()
{
	// Take over when the active spinner left the tree.
	if (web.is_valid() && !RenderingWeaver::get_singleton()->get_active_web_owner().is_valid())
	{
		_activate();
	}
	update_configuration_warnings();
}

PackedStringArray WeaverSpinner::get_configuration_warnings() const
{
	PackedStringArray warnings = Node::get_configuration_warnings();

	if (web.is_null())
	{
		warnings.push_back("No WeaverWeb is set.");
		return warnings;
	}

	const RenderingWeaver *weaver = RenderingWeaver::get_singleton();
	if (is_inside_tree() && weaver != nullptr && weaver->get_active_web_owner().is_valid() && weaver->get_active_web_owner() != get_instance_id())
	{
		warnings.push_back("Another WeaverSpinner is already active. This one is ignored until that one leaves the tree.");
	}

	const Array issues = web->validate();
	for (int i = 0; i < issues.size(); i++)
	{
		const Dictionary issue = issues[i];
		if (String(issue["severity"]) == "error")
		{
			warnings.push_back("WeaverWeb: " + String(issue["message"]));
		}
	}

	return warnings;
}
