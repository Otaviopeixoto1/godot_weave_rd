#include "weaver_web_node.h"
#include "weaver_profiler.h"

#include "core/object/class_db.h"



void WeaverWebNode::set_enabled(bool p_enabled) {
	enabled = p_enabled;
	emit_changed();
}

bool WeaverWebNode::get_enabled() const {
	return enabled;
}

void WeaverWebNode::set_stage(Stage p_stage) {
	ERR_FAIL_INDEX(p_stage, MAX);
	stage = p_stage;
	emit_changed();
}

WeaverWebNode::Stage WeaverWebNode::get_stage() const {
	return stage;
}

void WeaverWebNode::set_access_resolved_color(bool p_enabled) {
	access_resolved_color = p_enabled;
	emit_changed();
}

bool WeaverWebNode::get_access_resolved_color() const {
	return access_resolved_color;
}

void WeaverWebNode::set_access_resolved_depth(bool p_enabled) {
	access_resolved_depth = p_enabled;
	emit_changed();
}

bool WeaverWebNode::get_access_resolved_depth() const {
	return access_resolved_depth;
}

void WeaverWebNode::set_needs_motion_vectors(bool p_enabled) {
	needs_motion_vectors = p_enabled;
	emit_changed();
}

bool WeaverWebNode::get_needs_motion_vectors() const {
	return needs_motion_vectors;
}

void WeaverWebNode::set_needs_normal_roughness(bool p_enabled) {
	needs_normal_roughness = p_enabled;
	emit_changed();
}

bool WeaverWebNode::get_needs_normal_roughness() const {
	return needs_normal_roughness;
}

void WeaverWebNode::set_needs_separate_specular(bool p_enabled) {
	needs_separate_specular = p_enabled;
	emit_changed();
}

bool WeaverWebNode::get_needs_separate_specular() const {
	return needs_separate_specular;
}


// GDVIRTUAL_CALL returns false when no script or extension implements the method, then the native hook runs.
void WeaverWebNode::render(int p_stage, const RenderData *p_render_data) {
	if (!GDVIRTUAL_CALL(_render, p_stage, p_render_data)) {
		_render_native(p_stage, p_render_data);
	}
}

void WeaverWebNode::setup() {
	if (!GDVIRTUAL_CALL(_setup)) {
		_setup_native();
	}
}

void WeaverWebNode::cleanup() {
	if (!GDVIRTUAL_CALL(_cleanup)) {
		_cleanup_native();
	}
}

void WeaverWebNode::begin_capture(const String &p_name) {
	WeaverProfiler::begin_capture(p_name);
}

void WeaverWebNode::end_capture() {
	WeaverProfiler::end_capture();
}


void WeaverWebNode::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &WeaverWebNode::set_enabled);
	ClassDB::bind_method(D_METHOD("get_enabled"), &WeaverWebNode::get_enabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "get_enabled");

	ClassDB::bind_method(D_METHOD("set_stage", "stage"), &WeaverWebNode::set_stage);
	ClassDB::bind_method(D_METHOD("get_stage"), &WeaverWebNode::get_stage);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "stage", PROPERTY_HINT_ENUM, "Pre Opaque,Post Opaque,Post Sky,Pre Transparent,Post Transparent"), "set_stage", "get_stage");

	ClassDB::bind_method(D_METHOD("set_access_resolved_color", "enable"), &WeaverWebNode::set_access_resolved_color);
	ClassDB::bind_method(D_METHOD("get_access_resolved_color"), &WeaverWebNode::get_access_resolved_color);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "access_resolved_color"), "set_access_resolved_color", "get_access_resolved_color");

	ClassDB::bind_method(D_METHOD("set_access_resolved_depth", "enable"), &WeaverWebNode::set_access_resolved_depth);
	ClassDB::bind_method(D_METHOD("get_access_resolved_depth"), &WeaverWebNode::get_access_resolved_depth);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "access_resolved_depth"), "set_access_resolved_depth", "get_access_resolved_depth");

	ClassDB::bind_method(D_METHOD("set_needs_motion_vectors", "enable"), &WeaverWebNode::set_needs_motion_vectors);
	ClassDB::bind_method(D_METHOD("get_needs_motion_vectors"), &WeaverWebNode::get_needs_motion_vectors);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "needs_motion_vectors"), "set_needs_motion_vectors", "get_needs_motion_vectors");

	ClassDB::bind_method(D_METHOD("set_needs_normal_roughness", "enable"), &WeaverWebNode::set_needs_normal_roughness);
	ClassDB::bind_method(D_METHOD("get_needs_normal_roughness"), &WeaverWebNode::get_needs_normal_roughness);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "needs_normal_roughness"), "set_needs_normal_roughness", "get_needs_normal_roughness");

	ClassDB::bind_method(D_METHOD("set_needs_separate_specular", "enable"), &WeaverWebNode::set_needs_separate_specular);
	ClassDB::bind_method(D_METHOD("get_needs_separate_specular"), &WeaverWebNode::get_needs_separate_specular);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "needs_separate_specular"), "set_needs_separate_specular", "get_needs_separate_specular");

	ClassDB::bind_method(D_METHOD("begin_capture", "name"), &WeaverWebNode::begin_capture);
	ClassDB::bind_method(D_METHOD("end_capture"), &WeaverWebNode::end_capture);

	GDVIRTUAL_BIND(_render, "stage", "render_data");
	GDVIRTUAL_BIND(_setup);
	GDVIRTUAL_BIND(_cleanup);

	BIND_ENUM_CONSTANT(PRE_OPAQUE);
	BIND_ENUM_CONSTANT(POST_OPAQUE);
	BIND_ENUM_CONSTANT(POST_SKY);
	BIND_ENUM_CONSTANT(PRE_TRANSPARENT);
	BIND_ENUM_CONSTANT(POST_TRANSPARENT);
	BIND_ENUM_CONSTANT(MAX);
}
