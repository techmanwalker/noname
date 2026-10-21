#include "soundio/callbacks.hpp"
#include "soundio/writepass.hpp"
#include "ringbuffer/ringbuffer.hpp"

#include <soundio/soundio.h>

// The only TU that knows both ends: libsoundio's C callbacks on one side, the real ring on the other.
static_assert(rt_feed<audio_ring_buffer::rt_end>);

namespace {

audio_ring_buffer::rt_end
rt_of (SoundIoOutStream *outstream) noexcept
{
    return static_cast<audio_ring_buffer *>(outstream->userdata)->make_rt_end();
}

} // namespace

/**
    @brief Output callback for libsoundio.

    @details Executed at a very high priority.
    Must never block, nor allocate (no new nor std::vector), nor print to console.
*/
void
write_callback (struct SoundIoOutStream *outstream, [[maybe_unused]] int frame_count_min, int frame_count_max)
{
    auto rt = rt_of(outstream);
    rt_cb::run_write_callback(rt, outstream, frame_count_max);
}

void
error_callback (struct SoundIoOutStream *outstream, int err)
{
    rt_of(outstream).notify_error(rt_event::stream_error, err);
}

void
underflow_callback (struct SoundIoOutStream *outstream)
{
    rt_of(outstream).notify(rt_event::underflow);
}