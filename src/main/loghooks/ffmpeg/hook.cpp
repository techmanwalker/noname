#include "hook.hpp"

#include <QString>

#include <string>

extern "C" {
#include <libavutil/log.h>
}

Q_LOGGING_CATEGORY(l_ffmpeg, "ffmpeg")

namespace {

QtMsgType
to_qt_severity(int level)
{
    if (level <= AV_LOG_FATAL)   return QtCriticalMsg; // panic, fatal
    if (level <= AV_LOG_WARNING) return QtWarningMsg;  // error, warning
    if (level <= AV_LOG_INFO)    return QtInfoMsg;
    return QtDebugMsg;                                 // verbose, debug, trace
}

// Highest FFmpeg level worth generating, given what the category lets through.
int
level_from_qt_rules()
{
    const QLoggingCategory &c = l_ffmpeg();

    if (c.isDebugEnabled())    return AV_LOG_DEBUG;
    if (c.isInfoEnabled())     return AV_LOG_INFO;
    if (c.isWarningEnabled())  return AV_LOG_WARNING;
    if (c.isCriticalEnabled()) return AV_LOG_FATAL;
    return AV_LOG_QUIET;
}

void
emit_line(QtMsgType type, const char *line)
{
    const QString text = QString::fromUtf8(line);

    switch (type) {
    case QtCriticalMsg: qCCritical(l_ffmpeg).noquote() << text; break;
    case QtWarningMsg:  qCWarning(l_ffmpeg).noquote()  << text; break;
    case QtInfoMsg:     qCInfo(l_ffmpeg).noquote()     << text; break;
    default:            qCDebug(l_ffmpeg).noquote()    << text; break;
    }
}

// May be called from any thread that touches FFmpeg.
void
callback(void *avcl, int level, const char *fmt, va_list vl)
{
    level &= 0xff; // strip AV_LOG_C() flags

    if (level > av_log_get_level()) {
        return;
    }

    const QtMsgType type = to_qt_severity(level);
    if (!l_ffmpeg().isEnabled(type)) {
        return;
    }

    // One line may arrive over several calls: coalesce per thread until '\n'.
    thread_local std::string pending;
    thread_local int print_prefix = 1;

    char buf[1024];
    av_log_format_line2(avcl, level, fmt, vl, buf, sizeof buf, &print_prefix);

    pending += buf;
    if (pending.empty() || pending.back() != '\n') {
        return;
    }

    pending.pop_back();
    emit_line(type, pending.c_str());
    pending.clear();
}

}

namespace ffmpeg_log {

void
install()
{
    av_log_set_level(level_from_qt_rules());
    av_log_set_callback(callback);
}

void
uninstall()
{
    av_log_set_callback(av_log_default_callback);
}

}
