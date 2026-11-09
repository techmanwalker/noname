#pragma once

#include <QJsonObject>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(l_coverprovider)

namespace covers::lightness {
    struct lightness_table;
}

namespace debug {

QJsonObject
serialize (const covers::lightness::lightness_table &table);

}
