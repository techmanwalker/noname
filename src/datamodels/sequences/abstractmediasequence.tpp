#pragma once

#include "abstractmediasequence.hpp"
#include "mediatypes.hpp"

#include <QList>
#include <qabstractitemmodel.h>
#include <qlist.h>

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

template <media_type T>
QStringList
AbstractMediaSequence::sources() const
{
    QReadLocker locker(&_lock());
    QStringList uri_sources;

    QList<T> filtered_items = items<T>();

    uri_sources.reserve(static_cast<qsizetype>(_items().size()));

    for (const Types::Any &item : _items()) {
        if (const auto *resolved = std::get_if<T>(&item)) {
            if constexpr (std::is_same_v<T, Types::Song>) {
                uri_sources.append(resolved->source.toLocalFile());
            }
        }
    }

    return uri_sources;
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