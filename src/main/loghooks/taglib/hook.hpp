#pragma once

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(l_taglib)

namespace taglib_log {

// Route TagLib's process-wide debug output into the "taglib" category.
// Call once, before any TagLib use.
void install();

// Restore TagLib's default stderr output.
// Call only after every thread that uses TagLib has finished.
void uninstall();

}
