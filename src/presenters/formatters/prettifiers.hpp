#pragma once

#include "mediatypes.hpp"
#include <type_traits>

// Formatters, sorters, etc.

namespace Prettifiers {

    template <typename FieldType, typename MediaType>
    requires (
        std::is_convertible_v<MediaType, Types::Any>
    &&  (
            std::is_convertible_v<FieldType, QString>
        ||  std::is_convertible_v<FieldType, quint64>
        )
    )
    QList<MediaType> sortBy (FieldType MediaType::* member, QList<MediaType> list_to_sort, bool descending = false);
}

#include "prettifiers.tpp"