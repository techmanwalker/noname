#pragma once

#include "basicmediasequence.hpp"

template <media_type T>
QPersistentModelIndex
BasicMediaSequence<T>::append (const T &item)
{
    return AbstractMediaSequence::append(item);
}

template <media_type T>
QList<QPersistentModelIndex>
BasicMediaSequence<T>::batch_append(const QList<T> &items)
{
    return AbstractMediaSequence::batch_append(items);
}

template <media_type T>
QList<T>
BasicMediaSequence<T>::items () const
{
    return AbstractMediaSequence::items<T>();
}