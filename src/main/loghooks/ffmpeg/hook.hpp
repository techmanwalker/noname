#pragma once

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(l_ffmpeg)

namespace ffmpeg_log {

// Route FFmpeg's process-wide log into the "ffmpeg" category.
// Call once, after the Qt filter rules are set and before any FFmpeg use.
// FFmpeg's own level is derived from the category's enabled severities here.
void install();

// Restore FFmpeg's default stderr callback.
// Call only after every thread that uses FFmpeg has finished.
void uninstall();

}
