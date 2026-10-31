#pragma once

#include "weaver_view.h"

#include "core/io/resource.h"
#include "core/variant/typed_array.h"
#include "core/object/object.h"
#include "core/object/class_db.h"



class WeaverWeb : public Resource
{
	GDCLASS(WeaverWeb, Resource);

protected:
	static void _bind_methods();
public:
	WeaverWeb() = default;
	virtual ~WeaverWeb() = default;

	void set_weaver_views(const TypedArray<WeaverView> &p_weaver_compositors);
	TypedArray<WeaverView> get_weaver_views() const;

	// Checks the description for problems that would stop it from binding to a scene.
	// Returns an Array of Dictionary { "severity": "error"|"warning", "message": String, "view": int, "lane": int } (-1 when not specific). "lane" is the WeaverSpider index in the view.
	Array validate() const;

private:
	// APPROACH: BUILD THE WEB, CHECK THE SCENE:
	// All we need is to inject the WeaverWebCompositors into cameras (and WorldEnvironment)
	// We must make the web independent of scene tree. Each WeaverSpinner will be added as child to Camera3D. Each will hold a reference to the WeaverWeb
	// EACH CAMERA SUBSCRIBES TO THE WEB USING AN ID/PORT. We can then do this subscription USING THE RenderingWeaver (SERVER) for checking for the port
	// THE Web will abstract viewports as well. Each camera can be subscribed to a single viewport in the web and during runtime we can check which viewports are used in the scene
	// ---> We can also make a custom Viewport-related node that also subscribes to the web through am ID/PORT and then we check validity
	// ---> For the user, knowing which camera is which will be very important so we ABSOLUTELY NEED TO HOLD CAMERA REFERENCES (in the scene these will correspond to WeaverSpinners)
	//
	//
	// - THIS IS A GOOD APPROACH SINCE THE WEB BECOMES THE REAL SOURCE OF TRUTH (The scene is just the runtime component that gets validated)
	// - Each WeaverSpinner will subscribe a camera to a WeaverViewport through a Port.
	// -But we ALSO NEED TO IDENTIFY THE VIEWPORT THAT THE CAMERA REGISTERS TO. Each viewpoort on the web will have its set of ports
	//  First we can check if the viewport is the root one. The root viewport gets special treatment on the web (its the default one that will be pre-generated)
	//  THEN IF NOT THE ROOT WE CHECK IF ITS A WeaverVieport and THROUGH ITS OWNED RESOURCE WE IDENTIFY WHICH PORTS ARE AVAILABLE
	LocalVector<Ref<WeaverView>> weaver_views; // One WeaverView per viewport... The default one is ALWAYS at element 0...


	// A Weaver view should be initialized first, then its web should be initialized !

};
