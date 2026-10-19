#pragma once

#include "abstractmediasequence.hpp"

#include <concepts>

/**
    @brief Typed facade for a sequence that only holds T. NOT a Q_OBJECT and NOT a base-of-record:
    a leaf derives from AbstractMediaSequence FIRST (so moc/qmltyperegistrar can resolve its QML
    prototype) and from this SECOND, then pulls these names in with using-declarations.
*/
template <typename Derived, typename T>
class BasicSequenceOps
{
    static_assert(media_type<T>);

    // Derived is incomplete while the base list is parsed, so this can only be checked here
    Derived &self ()
    {
        static_assert(std::derived_from<Derived, BasicSequenceOps>,
                      "BasicSequenceOps<Derived, T>: Derived must be the LEAF class that inherits this facade");
        if constexpr (std::derived_from<Derived, BasicSequenceOps>) return static_cast<Derived &>(*this);
        else __builtin_unreachable(); // keeps the static_assert above the only diagnostic
    }
    const Derived &self () const
    {
        static_assert(std::derived_from<Derived, BasicSequenceOps>,
                      "BasicSequenceOps<Derived, T>: Derived must be the LEAF class that inherits this facade");
        if constexpr (std::derived_from<Derived, BasicSequenceOps>) return static_cast<const Derived &>(*this);
        else __builtin_unreachable();
    }

public:
    QPersistentModelIndex        append       (const T &item)         { return self().AbstractMediaSequence::append(item); }
    QList<QPersistentModelIndex> batch_append (const QList<T> &items) { return self().AbstractMediaSequence::batch_append(items); }
    QList<T>                     items        () const                { return self().AbstractMediaSequence::template items<T>(); }
};

// Pulls the typed facade into a leaf's scope. Both bases declare append/batch_append/items,
// so without these the call is ambiguous. Use inside the leaf's PUBLIC section; the caller
// supplies the trailing ';'. Derived is the LEAF class being declared; both arguments must
// match its BasicSequenceOps<...> base exactly.
#define USING_SEQUENCE_OPS(Derived, T)                 \
    using BasicSequenceOps<Derived, T>::append;        \
    using BasicSequenceOps<Derived, T>::batch_append;  \
    using BasicSequenceOps<Derived, T>::items
