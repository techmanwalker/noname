#pragma once

#include "abstractmediasequence.hpp"
#include "mediatypes.hpp"

#include <QList>

// --- readers

template <typename media_type>
QList<media_type> 
AbstractMediaSequence::items() const 
{
    QList<media_type> filtered_list;

    QReadLocker locker (&_lock());
    
    filtered_list.reserve(_items().size());

    for (const Types::Any &item : _items()) {
        if (std::holds_alternative<media_type>(item)) {
            filtered_list.append(std::get<media_type>(item));
        }
    }

    return filtered_list;
}

template <typename media_type>
QStringList 
AbstractMediaSequence::sources() const
{
    QReadLocker locker(&_lock());
    QStringList uri_sources;

    for (const Types::Any &item : _items()) {
        if (const auto *resolved = std::get_if<media_type>(&item)) {
            if constexpr (std::is_same_v<media_type, Types::Song>) {
                uri_sources.append(resolved->source.toLocalFile());
            }
        }
    }

    return uri_sources;
}

// proxy for both sources
template <typename Container>
requires
    std::ranges::forward_range<Container>
&&  std::convertible_to<typename Container::value_type, Types::Any>
QStringList
AbstractMediaSequence::sources (const Container &items)
{
    QStringList uri_sources;
    uri_sources.reserve(items.size());

    for (const Types::Any &item : std::as_const(items)) {
        std::visit([&uri_sources](const auto &resolved_item) { 
            using T = std::decay_t<decltype(resolved_item)>;
            
            if constexpr (std::is_same_v<T, Types::Song>) {
                uri_sources.append(resolved_item.source.toLocalFile());
            }
        }, item);
    }

    return uri_sources;
}

template <typename MediaType, typename FieldType>
QPersistentModelIndex 
AbstractMediaSequence::find(FieldType MediaType::* member, const FieldType &needle) const
{
    QReadLocker locker (&_lock());

    for (size_t i = 0; i < _items().size(); ++i) {
        if (const MediaType *actual_media = std::get_if<MediaType>(&_items().at(i))) {
            if (actual_media->*member == needle) {
                return index(static_cast<int>(i));
            }
        }
    }

    return QPersistentModelIndex();
}


// --- writers

template <typename Container>
requires
    std::ranges::forward_range<Container>
&&  std::convertible_to<typename Container::value_type, Types::Any>
QList<QPersistentModelIndex>
AbstractMediaSequence::batch_append(const Container &items)
{
    QList<QPersistentModelIndex> indices; 
    
    if (items.empty()) return indices;

    std::vector<Types::Any> valid_items;
    valid_items.reserve(items.size());

    for (const Types::Any &item : items) {
        if (
            std::holds_alternative<Types::Song>(item)
        &&  !std::get<Types::Song>(item).is_valid())
        {
            continue;
        }

        valid_items.push_back(item);
    }

    if (valid_items.empty()) return indices;

    int first_row = rowCount();
    int last_row = first_row + static_cast<int>(valid_items.size()) - 1;

    beginInsertRows({}, first_row, last_row);
    {
        QWriteLocker locker(&_lock());
        _items().reserve(_items().size() + valid_items.size());

        for (Types::Any &valid_item : valid_items) {
            _items().push_back(std::move(valid_item));
        }
    }
    endInsertRows();

    for (int r = first_row; r <= last_row; ++r) {
        indices.append(index(r));
    }

    emit countChanged();

    return indices;
}