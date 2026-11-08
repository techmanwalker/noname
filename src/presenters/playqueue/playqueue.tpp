#pragma once

#include "playqueue.hpp"

template <media_type T>
bool
PlayQueueLI::pointed_to_t (const QPersistentModelIndex &idx, std::invocable<T> auto callback)
{
    return _sequence().pointed_to<T>(to_sequence(idx), callback);
}