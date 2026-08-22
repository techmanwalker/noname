#pragma once

// mediatypes.hpp must stay since Types::Any is exposed through public method signatures
#include "mediatypes.hpp"

#include <QAbstractItemModel>
#include <QFuture>
#include <QLoggingCategory>
#include <QReadWriteLock>

#include <QtQmlIntegration/qqmlintegration.h>

#include <functional>
#include <memory>

Q_DECLARE_LOGGING_CATEGORY(l_mediasequences)

class AbstractMediaSequencePrivate;

/**
    @brief List of any form of playable media, enumerated in the Types:: namespace. 
    This is not intended to be instantiated directly but rather to use one of the inherited classes.

    Inherited classes can restrict themselves to only hold 1 or more specified types and update
    their add(T) and remove(T) functions accordingly.
*/
class AbstractMediaSequence : public QAbstractListModel {
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY (qsizetype count READ rowCount NOTIFY countChanged)

public:
    // rolecompiler.hpp dropped by utilizing std::function directly
    explicit AbstractMediaSequence(QObject *parent, std::vector<std::pair<QByteArray, std::function<QVariant(const Types::Any &)>>> role_defs);
    ~AbstractMediaSequence() override;

    int                    rowCount(const QModelIndex &parent = QModelIndex())        const override;
    QVariant               data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames()                                                const override;

    std::optional<int>     roleNumber(const QByteArray &role) const;

    const QList<Types::Any> & items() const;

    // fetch sources of any input list
    template <typename Container>
    requires
        std::ranges::forward_range<Container> // Any type of list
    &&  std::convertible_to<typename Container::value_type, Types::Any> // that can be contained by internal list
    static QStringList sources (const Container &items);

    // fetch the source of every element
    QStringList sources () const;

    // fetch the source of every element convertible to media_type
    template <typename media_type>
    QStringList sources() const;

    // position solvers
    std::optional<std::reference_wrapper<Types::Any>> pointed_to (const QPersistentModelIndex &idx);
    std::optional<size_t> row_pointed_to(const QPersistentModelIndex &idx) const;

    QPersistentModelIndex index_next_to (const QPersistentModelIndex &idx) const;
    std::optional<std::reference_wrapper<Types::Any>> next_to (const QPersistentModelIndex &idx);
    std::optional<size_t> row_next_to(const QPersistentModelIndex &idx) const;

    // Access to the raw item for inherited classes that need extra roles
    std::optional<std::reference_wrapper<Types::Any>> item_at(size_t index);

    int itemCount() const;

    // read a role directly
    Q_INVOKABLE QVariant readRole(qsizetype row, const QByteArray &role) const;

    // items that can be converted to certain type
    template <typename media_type>
    QList<media_type> items() const;

    // Find an item whose pointed member matches the needle, returns invalid if not found
    template <typename MediaType, typename FieldType>
    QPersistentModelIndex find(FieldType MediaType::* member, const FieldType &needle) const;

signals:
    void countChanged();

protected:
    // Inherited models call these to manipulate the container
    QPersistentModelIndex append(const Types::Any &item);

    template <typename Container>
    requires
        std::ranges::forward_range<Container>
    &&  std::convertible_to<typename Container::value_type, Types::Any>
    QList<QPersistentModelIndex>
    batch_append(const Container &items);

    QFuture<void> progressive_batch_append (QList<QFuture<Types::Any>> futures);

    // remove items in batch using their persistent indices
    void batch_remove (const QList<QPersistentModelIndex> &items);

    std::optional<size_t> __row_pointed_to_unlocked (const QPersistentModelIndex &idx) const;
    std::optional<size_t> __row_next_to_unlocked (const QPersistentModelIndex &idx) const;
    QPersistentModelIndex __index_next_to_unlocked (const QPersistentModelIndex &idx) const;

    void remove(size_t index);
    void clear();

private:
    std::unique_ptr<AbstractMediaSequencePrivate> m_d;

    // Private reference bypasses to permit abstractmediasequence.tpp logic
    // avoiding the dependency on complete types from the Pimpl design.
    QList<Types::Any>& _items();
    const QList<Types::Any>& _items() const;
    QReadWriteLock& _lock() const;
};

#include "abstractmediasequence.tpp"