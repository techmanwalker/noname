#pragma once

#include "mediatypes.hpp"
#include <type_traits>

// Formatters, sorters, etc.

namespace MediaTransforms {

    template <typename FieldType, media_type MediaType>
    requires (
        std::is_convertible_v<FieldType, QString>
    ||  std::is_convertible_v<FieldType, quint64>
    )
    QList<MediaType> sort_by (FieldType MediaType::* member, QList<MediaType> list_to_sort, bool descending = false);
}

#include "sort.tpp"