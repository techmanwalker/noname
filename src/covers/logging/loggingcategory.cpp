#include "logging/l_coverprovider.hpp"
#include "presentation/lightness.hpp"

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

    // row-major: [row][column]; blurred mean lightness of each cell
    QJsonArray rows;
    for (size_t r = 0; r < lightness_table::s_rows; ++r) {
        QJsonArray cells;
        for (size_t c = 0; c < lightness_table::s_columns; ++c) {
            cells.append(table.cell(r, c));
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
