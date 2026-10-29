#pragma once

#include "scene/main/node.h"
#include "resources/weaver_web.h"

//
// Activates a WeaverWeb while it is in the tree. Only one WeaverSpinner can hold the active Web (a second one waits and
// takes over when the active one leaves the tree)
//
class WeaverSpinner : public Node
{
	GDCLASS(WeaverSpinner, Node);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_web(const Ref<WeaverWeb> &p_web);
	Ref<WeaverWeb> get_web() const;

	PackedStringArray get_configuration_warnings() const override;

private:
	Ref<WeaverWeb> web;

	void _activate();
	void _on_bindings_changed();
};
