#include "weaver_profiler.h"

#include "core/debugger/engine_debugger.h"
#include "core/variant/variant.h"
#include "servers/rendering/rendering_device.h"

static const char *PROFILER_NAME = "weaver_profiler";
static const char *FRAMES_MESSAGE = "weaver_profiler:frames";

// Marks the timestamps of the markers, the list also has the engine's own.
static const char *BEGIN_PREFIX = "weaver_begin:";
static const char *END_NAME = "weaver_end";


void WeaverProfiler::initialize()
{
	EngineDebugger::register_profiler(PROFILER_NAME, EngineDebugger::Profiler(nullptr, &WeaverProfiler::_toggle, nullptr, &WeaverProfiler::_tick));
}

void WeaverProfiler::finalize()
{
	if (EngineDebugger::has_profiler(PROFILER_NAME))
	{
		EngineDebugger::unregister_profiler(PROFILER_NAME);
	}
}

void WeaverProfiler::begin_capture(const String &p_name)
{
	RenderingDevice *rd = RenderingDevice::get_singleton();
	if (!is_capturing() || rd == nullptr)
	{
		return;
	}

	_read_results(rd);
	rd->capture_timestamp(String(BEGIN_PREFIX) + p_name);
}

void WeaverProfiler::end_capture()
{
	RenderingDevice *rd = RenderingDevice::get_singleton();
	if (!is_capturing() || rd == nullptr)
	{
		return;
	}

	rd->capture_timestamp(END_NAME);
}

void WeaverProfiler::_read_results(RenderingDevice *p_rd)
{
	const uint64_t frame = p_rd->get_captured_timestamps_frame();
	if (frame == last_read_frame)
	{
		return;
	}
	last_read_frame = frame;

	CapturedFrame captured;
	String open_name;
	uint64_t open_time = 0;

	const uint32_t count = p_rd->get_captured_timestamps_count();
	for (uint32_t i = 0; i < count; i++)
	{
		const String name = p_rd->get_captured_timestamp_name(i);
		if (name.begins_with(BEGIN_PREFIX))
		{
			open_name = name.substr(String(BEGIN_PREFIX).length());
			open_time = p_rd->get_captured_timestamp_gpu_time(i);
		}
		else if (name == END_NAME && !open_name.is_empty())
		{
			const uint64_t time_ns = p_rd->get_captured_timestamp_gpu_time(i) - open_time;
			captured.names.push_back(open_name);
			captured.times_ms.push_back(float(double(time_ns) / 1000000.0));
			open_name = String();
		}
	}

	if (captured.names.is_empty())
	{
		return;
	}

	MutexLock lock(pending_mutex);
	pending_frames.push_back(captured);
}

void WeaverProfiler::_toggle(void *p_user, bool p_enable, const Array &p_opts)
{
	capturing.set_to(p_enable);

	MutexLock lock(pending_mutex);
	pending_frames.clear();
}

void WeaverProfiler::_tick(void *p_user, double p_frame_time, double p_process_time, double p_physics_time, double p_physics_frame_time)
{
	Array frames;
	{
		MutexLock lock(pending_mutex);
		for (const CapturedFrame &captured : pending_frames)
		{
			Array frame;
			frame.push_back(captured.names);
			frame.push_back(captured.times_ms);
			frames.push_back(frame);
		}
		pending_frames.clear();
	}

	if (!frames.is_empty())
	{
		EngineDebugger::get_singleton()->send_message(FRAMES_MESSAGE, frames);
	}
}
