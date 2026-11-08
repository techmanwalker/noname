#pragma once

// mediatypes.hpp must stay since Types::Any is exposed through public method signatures
#include "mediaconcepts.hpp"
#include "mediatypes.hpp"

#include <QAbstractItemModel>
#include <QFuture>
#include <QLoggingCategory>
#include <QReadWriteLock>

#include <QtQmlIntegration>

#include <functional>
#include <memory>

Q_DECLARE_LOGGING_CATEGORY(l_mediasequences)

class AbstractMediaSequencePrivate;

template <typename Derived, typename T> class BasicSequenceOps; // typed facade, see basicsequenceops.hpp

/**
    @brief List of any form of playable media, enumerated in the Types:: namespace. 
    This is not intended to be instantiated directly but rather to use one of the inherited classes.

    Inherited classes can restrict themselves to only hold 1 or more specified types and update
    their add(T) and remove(T) functions accordingly.
*/
class AbstractMediaSequence : public QAbstractListModel
{
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY (qsizetype count READ rowCount NOTIFY count_changed)

public:
    // rolecompiler.hpp dropped by utilizing std::function directly
    explicit AbstractMediaSequence(QObject *parent, const std::vector<std::pair<QByteArray, std::function<QVariant(const Types::Any &)>>> &role_defs);
    ~AbstractMediaSequence() override;

    int                    rowCount(const QModelIndex &parent = QModelIndex())        const override;
    QVariant               data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames()                                                const override;

    std::optional<int>     role_number(const QByteArray &role) const;

    QList<Types::Any> items() const;

    // items that can be converted to certain types
    // returns one QList per requested type, in the order requested, in a single pass
    template <media_type T>
    QList<T> items () const;

    // fetch sources of any input list
    template <media_sequence container>
    static QStringList sources (const container &items);

    // fetch the source of every element
    QStringList sources () const;

    // position solvers, lifetime control by predicate

    template <media_type T>
    bool pointed_to (const QPersistentModelIndex &idx, std::invocable<T> auto callback);

    // read a role directly
    QVariant read_role(qsizetype row, const QByteArray &role) const;

    template <typename... Ts> requires distinct_media_types<Ts...>
    std::tuple<QList<Ts>...> items_of_types () const;

    // Find an item whose pointed member matches the needle, returns invalid if not found
    template <typename MediaType, typename FieldType>
    QPersistentModelIndex find(FieldType MediaType::* member, const FieldType &needle) const;

signals:
    void count_changed();

protected:
    // Inherited models call these to manipulate the container
    template <media_type T>
    QPersistentModelIndex append(T item);

    template <media_sequence container>
    QList<QPersistentModelIndex>
    batch_append(container items);

    QFuture<void> progressive_batch_append (QList<QFuture<Types::Any>> futures);

    // remove items in batch using their persistent indices
    void batch_remove (const QList<QPersistentModelIndex> &items);

    void remove(size_t index);
    void clear();

private:
    // the typed facade is the only outsider allowed to reach the protected writers
    template <typename, typename> friend class BasicSequenceOps;
    
    std::unique_ptr<AbstractMediaSequencePrivate> m_d;

    // Private reference bypasses to permit abstractmediasequence.tpp logic
    // avoiding the dependency on complete types from the Pimpl design.
    QList<Types::Any>& _items();
    const QList<Types::Any>& _items() const;
    QReadWriteLock& _lock() const;

    // must not be touched by anything that are not their locking supersets
    std::optional<size_t> __row_pointed_to_unlocked (const QPersistentModelIndex &idx) const;
    std::optional<std::reference_wrapper<Types::Any>>
                          __item_pointed_to_unlocked (const QPersistentModelIndex &idx) const;
    
    template <typename... Ts> requires distinct_media_types<Ts...>
    std::tuple<QList<Ts>...> __items_of_types_unlocked () const;
};

#include "abstractmediasequence.tpp"
