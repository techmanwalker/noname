#include "hook.hpp"

#include <QString>

#include <taglib/tdebuglistener.h>
#include <taglib/tstring.h>

Q_LOGGING_CATEGORY(l_taglib, "taglib")

namespace {

class qt_listener final : public TagLib::DebugListener
{
public:
    // Called from whichever thread touches TagLib.
    void printMessage(const TagLib::String &msg) override
    {
        // TagLib terminates every message with '\n'
        qCDebug(l_taglib).noquote() << QString::fromStdString(msg.to8Bit(true)).trimmed();
    }
};

}

namespace taglib_log {

void
install()
{
    static qt_listener listener; // TagLib keeps the pointer, so it must outlive its use
    TagLib::setDebugListener(&listener);
}

void
uninstall()
{
    TagLib::setDebugListener(nullptr);
}

}
