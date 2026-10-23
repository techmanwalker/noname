#pragma once

#include "abstractmediasequence.hpp"
#include "mediatypes.hpp"

#include <QList>
#include <qabstractitemmodel.h>
#include <qlist.h>
#include <qreadwritelock.h>
#include <variant>

// --- readers
template <typename... Ts>
    requires distinct_media_types<Ts...>
std::tuple<QList<Ts>...>
AbstractMediaSequence::__items_of_types_unlocked () const
{
    std::tuple<QList<Ts>...> result;

    for (const Types::Any &item : _items()) {
        std::visit([&]<typename V>(const V &value) {
            if constexpr ((std::same_as<V, Ts> || ...)) {
                std::get<QList<V>>(result).append(value);
            }
        }, item);
    }

    return result;
}

template <typename... Ts>
    requires distinct_media_types<Ts...>
std::tuple<QList<Ts>...>
AbstractMediaSequence::items_of_types () const
{
    QReadLocker locker (&_lock());
    return __items_of_types_unlocked<Ts...>();
}


template <media_type T>
QList<T> 
AbstractMediaSequence::items () const
{
    auto [list] = items_of_types<T>();

    return list;
}

// proxy for both sources
template <media_sequence container>
QStringList
AbstractMediaSequence::sources (const container &items)
{
    QStringList uri_sources;
    uri_sources.reserve(std::ranges::distance(items));

    for (const auto &item : items) {
        std::visit([&uri_sources](const auto &resolved_item) { 
            using T = std::decay_t<decltype(resolved_item)>;
            
            if constexpr (std::is_same_v<T, Types::Song>) {
                uri_sources.append(resolved_item.source.toLocalFile());
            }
        }, Types::Any{item});
    }

    return uri_sources;
}

template <typename MediaType, typename FieldType>
QPersistentModelIndex 
AbstractMediaSequence::find(FieldType MediaType::* member, const FieldType &needle) const
{
    QReadLocker locker (&_lock());

    for (qsizetype i = 0; i < _items().size(); ++i) {
        if (const MediaType *actual_media = std::get_if<MediaType>(&_items().at(i))) {
            if (actual_media->*member == needle) {
                return index(static_cast<int>(i));
            }
        }
    }

    return QPersistentModelIndex();
}


// --- writers

template <media_sequence container>
QList<QPersistentModelIndex>
AbstractMediaSequence::batch_append(const container &items)
{
       
    if (items.empty()) return {};

    std::vector<Types::Any> valid_items;
    valid_items.reserve(items.size());

    for (const auto &item : items) {
        if constexpr (std::is_same_v<std::decay_t<decltype(item)>, Types::Song>) {
            if (!item.is_valid()) {
                continue;
            }
        }

        valid_items.push_back(item);
    }

    if (valid_items.empty()) return {};

    int first_row = rowCount();
    int last_row = first_row + static_cast<int>(valid_items.size()) - 1;

    beginInsertRows({}, first_row, last_row);
    {
        QWriteLocker locker(&_lock());
        auto &target = _items();
        target.reserve(target.size() + valid_items.size());

        std::move(valid_items.begin(), valid_items.end(), std::back_inserter(target));
    }
    endInsertRows();

    QList<QPersistentModelIndex> indices; 
    indices.reserve(valid_items.size());
    for (int r = first_row; r <= last_row; ++r) {
        indices.emplace_back(index(r));
    }

    emit count_changed();

    return indices;
}

template <media_type T>
bool
AbstractMediaSequence::pointed_to(const QPersistentModelIndex &idx, std::invocable<T> auto callback)
{
    // use lock type based on const-ness
    constexpr bool is_const_target = std::is_const_v<std::remove_reference_t<T>>;
    using LockerType = std::conditional_t<is_const_target, QReadLocker, QWriteLocker>;
    LockerType locker (&_lock());

    // get item ref
    std::optional<std::reference_wrapper<Types::Any>> ref = __item_pointed_to_unlocked(idx);
    if (!ref.has_value()) return false;

    // base type
    using base_t = std::remove_cvref_t<T>;

    // reference to the real variant
    using variant_ref_type = std::conditional_t<is_const_target, const Types::Any&, Types::Any&>;
    variant_ref_type underlying_variant = ref.value().get();

    // a: T is exactly Types::Any
    if constexpr (std::is_same_v<base_t, Types::Any>) {
        using return_type = std::invoke_result_t<decltype(callback), variant_ref_type>;

        if constexpr (std::is_void_v<return_type>) {
            callback(underlying_variant);
            return true;
        } else {
            return callback(underlying_variant);
        }
    } 
    // b: T is an alternative for Types::Any
    else {
        if (!std::holds_alternative<base_t>(underlying_variant)) return false;

        // properly propagate constness (if variant_ref_type is const, std::get will return const base_t&)
        auto&& extracted_value = std::get<base_t>(underlying_variant);
        using return_type = std::invoke_result_t<decltype(callback), decltype(extracted_value)>;

        if constexpr (std::is_void_v<return_type>) {
            callback(std::forward<decltype(extracted_value)>(extracted_value));
            return true;
        } else {
            return callback(std::forward<decltype(extracted_value)>(extracted_value));
        }
    }
}