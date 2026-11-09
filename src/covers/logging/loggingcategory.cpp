#include "coverlogging.hpp"
#include "coverlightness.hpp"
#include "coverstorage.hpp"

#include <QJsonArray>

Q_LOGGING_CATEGORY(l_coverprovider, "coverprovider")

namespace debug {

QJsonObject
serialize (const covers::lightness::lightness_table &table)
{
    using covers::lightness::lightness_table;

    QJsonObject out;

    out["rows"]    = static_cast<qint64>(lightness_table::s_rows);
    out["columns"] = static_cast<qint64>(lightness_table::s_columns);

    // row-major: table[row][column], same layout as m_table
    QJsonArray rows;
    for (const auto &row : table.m_table) {
        QJsonArray cells;
        for (const double lightness : row) {
            cells.append(lightness);
        }
        rows.append(cells);
    }
    out["table"] = rows;

    out["aspect_ratio"] = QStringLiteral("%1:%2")
        .arg(table.m_aspect_ratio.first)
        .arg(table.m_aspect_ratio.second);

    return out;
}

}
