#pragma once

#include "prettifiers.hpp"

namespace Prettifiers {

template <typename FieldType, typename MediaType>
requires (
    std::is_convertible_v<MediaType, Types::Any>
&&  (
        std::is_convertible_v<FieldType, QString>
    ||  std::is_convertible_v<FieldType, quint64>
    )
)
QList<MediaType>
sortBy (FieldType MediaType::* member, QList<MediaType> list_to_sort, bool descending)
{
    // passed by value, returned by value

    // sort in ascending or descending order depending on the FieldType
    std::ranges::sort(list_to_sort, [member, descending](const MediaType &a, const MediaType &b) {
        const FieldType &field_a = a.*member;
        const FieldType &field_b = b.*member;

        if constexpr (std::is_convertible_v<FieldType, QString>) {
            // locale-aware compare, consistent with the "songs" role sort in defaultroles.hpp
            const int cmp = field_a.localeAwareCompare(field_b);
            return descending ? cmp > 0 : cmp < 0;
        } else {
            return descending ? field_a > field_b : field_a < field_b;
        }
    });

    QList<MediaType> converted;
    converted.reserve(list_to_sort.size());

    for (MediaType &item : list_to_sort) {
        // list_to_sort is our own local copy — move out of it rather than copying again
        converted.append(MediaType { std::move(item) });
    }

    return converted;
}

}