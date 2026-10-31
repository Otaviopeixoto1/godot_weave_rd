#pragma once

#include "core/os/mutex.h"
#include "core/string/ustring.h"
#include "core/templates/local_vector.h"
#include "core/templates/safe_refcount.h"
#include "core/variant/variant.h"

class RenderingDevice;

//
// WeaverProfiler measures the GPU time of the passes marked between WeaverWebNode::begin_capture() and WeaverWebNode::end_capture() and sends
// them to the editor through the debugger connection. Its registered as an EngineDebugger profiler, so the editor turns
// it on and off with EditorDebuggerSession.toggle_profiler(). While its off, the markers only check a flag.
//
// Timestamps are recorded and read on the RENDER THREAD and GPU results are only available a few frames later
//
class WeaverProfiler
{
public:
	static void initialize();
	static void finalize();

	static bool is_capturing() { return capturing.is_set(); }

	static void begin_capture(const String &p_name);
	static void end_capture();

private:
	struct CapturedFrame
	{
		PackedStringArray names;
		PackedFloat32Array times_ms;
	};

	static inline SafeFlag capturing;
	static inline uint64_t last_read_frame = 0; // Render thread only

	static inline Mutex pending_mutex;
	static inline LocalVector<CapturedFrame> pending_frames;

	static void _read_results(RenderingDevice *p_rd);

	static void _toggle(void *p_user, bool p_enable, const Array &p_opts);
	static void _tick(void *p_user, double p_frame_time, double p_process_time, double p_physics_time, double p_physics_frame_time);
};
