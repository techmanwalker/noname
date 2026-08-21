#pragma once

#include <QLoggingCategory>

// logging category

Q_DECLARE_LOGGING_CATEGORY(l_audioengine) // errors in audioengine itself
Q_DECLARE_LOGGING_CATEGORY(l_soundio) // soundio specific errors
Q_DECLARE_LOGGING_CATEGORY(l_ffmpeg) // errors in ffmpeg decoding