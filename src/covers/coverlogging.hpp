#pragma once

#include <QJsonObject>

namespace covers::luminance {
    struct table_luma;
}

namespace debug {

QJsonObject
serialize (const covers::luminance::table_luma &table);

}