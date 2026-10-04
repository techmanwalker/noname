#include "coverlogging.hpp"
#include "coverluminance.hpp"
#include "coverstorage.hpp"

#include <QJsonArray>

Q_LOGGING_CATEGORY(l_coverprovider, "noname.coverprovider")

namespace debug {

QJsonObject
serialize (const covers::luminance::table_luma &table)
{
    using covers::luminance::table_luma;

    QJsonObject out;

    out["rows"]    = static_cast<qint64>(table_luma::s_rows);
    out["columns"] = static_cast<qint64>(table_luma::s_columns);

    // row-major: table[row][column], same layout as m_table
    QJsonArray rows;
    for (const auto &row : table.m_table) {
        QJsonArray cells;
        for (const double luma : row) {
            cells.append(luma);
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