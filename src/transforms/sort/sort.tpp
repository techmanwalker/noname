#pragma once

#include "mediatypes.hpp"
#include "sort.hpp"

namespace MediaTransforms {

template <sortable_field FieldType>
bool
field_less (const FieldType &a, const FieldType &b, bool descending)
{
    if constexpr (std::is_convertible_v<FieldType, QString>) {
        // locale-aware compare, consistent with the "tracks" role sort in defaultroles.hpp
        const int cmp = a.localeAwareCompare(b);
        return descending ? cmp > 0 : cmp < 0;
    } else {
        return descending ? a > b : a < b;
    }
}

template <sortable_field FieldType, media_type MediaType>
QList<MediaType>
sort_by (FieldType MediaType::* member, QList<MediaType> list_to_sort, bool descending)
{
    // passed by value, returned by value

    // sort in ascending or descending order depending on the FieldType
    std::ranges::sort(list_to_sort, [member, descending](const MediaType &a, const MediaType &b) {
        return field_less(a.*member, b.*member, descending);
    });

    QList<MediaType> converted;
    converted.reserve(list_to_sort.size());

    for (MediaType &item : list_to_sort) {
        // list_to_sort is our own local copy — move out of it rather than copying again
        converted.append(std::move(item));
    }

    return converted;
}

}
