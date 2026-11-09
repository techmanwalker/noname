#pragma once

#include <QJsonObject>

namespace covers::lightness {
    struct lightness_table;
}

namespace debug {

QJsonObject
serialize (const covers::lightness::lightness_table &table);

}
