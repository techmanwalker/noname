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

    QReadLocker locker (&m_lock);
    
    filtered_list.reserve(m_items.size());

    for (const Types::Any &item : m_items) {
        // verify if the variant holds the type
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
    QReadLocker locker(&m_lock);
    QStringList uri_sources;

    for (const Types::Any &item : m_items) {
        // Check if the variant holds the requested type
        if (const auto *resolved = std::get_if<media_type>(&item)) {
            
            // Extract the field depending on what type was requested
            if constexpr (std::is_same_v<media_type, Types::Song>) {
                uri_sources.append(resolved->source.toLocalFile());
            }
        }
    }

    return uri_sources;
}

template <typename MediaType, typename FieldType>
QPersistentModelIndex 
AbstractMediaSequence::find(FieldType MediaType::* member, const FieldType &needle) const
{
    QReadLocker locker (&m_lock);

    for (size_t i = 0; i < m_items.size(); ++i) {
        
        // Pass the address of the variant to std::get_if to safely check its active type
        if (const MediaType *actual_media = std::get_if<MediaType>(&m_items.at(i))) {
            
            // Apply the pointer-to-member operator (->*) to evaluate the target field
            if (actual_media->*member == needle) {
                return index(static_cast<int>(i));
            }
        }
    }

    // invalid if not found
    return QPersistentModelIndex();
}


// --- writers

template <typename Container>
requires
    std::ranges::forward_range<Container> // Any type of list
&&  std::convertible_to<typename Container::value_type, Types::Any> // that can be contained by m_items
QList<QPersistentModelIndex>
AbstractMediaSequence::batch_append(const Container &items)
{
    // ready to use indices for right after manipulation
    QList<QPersistentModelIndex> indices; 
    
    if (items.empty()) return indices;

    // Filter only valid items before appending
    std::vector<Types::Any> valid_items;
    valid_items.reserve(items.size());

    for (const Types::Any &item : items) {

        // if such item is actually a Types::Song and it is not valid, continue;
        if (
            std::holds_alternative<Types::Song>(item)
        &&  !std::get<Types::Song>(item).is_valid())
        {
            continue;
        }

        valid_items.push_back(item);
    }

    if (valid_items.empty()) return indices;

    // Notify to QML views the entire block insertion at once
    int first_row = rowCount();
    int last_row = first_row + static_cast<int>(valid_items.size()) - 1;

    beginInsertRows({}, first_row, last_row);
    {
        QWriteLocker locker(&m_lock);
        m_items.reserve(m_items.size() + valid_items.size());

        for (Types::Any &valid_item : valid_items) {
            m_items.push_back(std::move(valid_item));
        }
    }
    endInsertRows();

    // Generate persistent model indices safely after layout update
    for (int r = first_row; r <= last_row; ++r) {
        indices.append(index(r));
    }

    emit countChanged();

    return indices;
}