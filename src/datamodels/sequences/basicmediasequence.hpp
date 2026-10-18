#pragma once

#include "abstractmediasequence.hpp"

// typed mixin to stop redefining templated functions everywhere

template <media_type T>
class BasicMediaSequence : public AbstractMediaSequence
{
public:
    using AbstractMediaSequence::AbstractMediaSequence; // inherit (parent, role_defs)

    QPersistentModelIndex        append       (const T &item);
    QList<QPersistentModelIndex> batch_append (const QList<T> &items);
    QList<T>                     items        () const;
};

#include "basicmediasequence.tpp"