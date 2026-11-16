#pragma once

#include "mediatypes.hpp"
#include "mediaconcepts.hpp"

// Formatters, sorters, etc.

namespace MediaTransforms {
    template <sortable_field FieldType>
    bool field_less (const FieldType &a, const FieldType &b, bool descending);

    template <sortable_field FieldType, media_type MediaType>
    QList<MediaType> sort_by (FieldType MediaType::* member, QList<MediaType> list_to_sort, bool descending = false);
}

#include "sort.tpp"
