#pragma once

// libsoundio entry points. The loop behind them lives in soundio/writepass.hpp.
void write_callback     (struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max);
void error_callback     (struct SoundIoOutStream *outstream, int err);
void underflow_callback (struct SoundIoOutStream *outstream);