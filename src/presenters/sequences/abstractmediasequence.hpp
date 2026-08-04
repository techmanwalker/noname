#pragma once

#include "coverprovider.hpp"
#include "defaultroles.hpp"
#include "mediatypes.hpp"

#include <QAbstractItemModel>
#include <QFuture>
#include <QLoggingCategory>
#include <QReadWriteLock>

#include <QtQmlIntegration/qqmlintegration.h>
 
#include <rapidfuzz/fuzz.hpp>


Q_DECLARE_LOGGING_CATEGORY(l_mediasequences)


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

private:
    CompiledRoleSet           m_roles;
    QList<Types::Any>         m_items;

public:
    explicit AbstractMediaSequence(QObject *parent, std::vector<std::pair<QByteArray, RoleExtractor>> role_defs);

    int                    rowCount(const QModelIndex &parent = QModelIndex())        const override;
    QVariant               data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames()                                                const override;

    std::optional<int>     roleNumber(const QByteArray &role) const;

    const decltype(m_items) & items() const;

    // proxy for both sources
    static QStringList sources (const QList<Types::Any> &items);
    std::optional<std::reference_wrapper<Types::Any>> pointed_to(const QPersistentModelIndex &idx);
    std::optional<size_t> row_pointed_to(const QPersistentModelIndex &idx) const;

    QStringList sources () const; // return the source or path component of all items

    // Access to the raw item for inherited classes that need extra roles
    std::optional<std::reference_wrapper<Types::Any>> item_at(size_t index);
    int itemCount() const;

    // read a role directly
    Q_INVOKABLE QVariant readRole(qsizetype row, const QByteArray &role) const;

    // items that can be converted to certain type
    template <typename media_type>
    QList<media_type> items() const;

    // sources of convertibles to this type
    template <typename media_type>
    QStringList sources() const;

    // Find an item whose pointed member matches the needle, returns invalid if not found
    template <typename MediaType, typename FieldType>
    QPersistentModelIndex find(FieldType MediaType::* member, const FieldType &needle) const;

    static std::string normalize_string_for_search (const QString &str);
    
    QList<QPersistentModelIndex>
    search_by_title (
        const QString &keywords, // search tokens, QString has wider locale support
        double score_thresh = 50 // out of 100
    );

signals:
    void countChanged();

protected:
    // Inherited models call these to manipulate the container
    QPersistentModelIndex append(const Types::Any &item);

    template <typename Container>
    requires
        std::ranges::forward_range<Container> // Any type of list
    &&  std::convertible_to<typename Container::value_type, Types::Any> // that can be contained by m_items
    QList<QPersistentModelIndex>
    batch_append(const Container &items);

    // only extract songs metadata
    QFuture<void> batch_append (const QList<QUrl> &sources, std::shared_ptr<covers::live::cover_provider> provider);

    /*  Append items one at a time as each future resolves, instead of waiting for the whole
    batch — for progressive/incremental loading (e.g. song_factory emitting results as
    they're produced, rather than all at once). Arrival order doesn't matter: consumers
    that care about order already re-sort on read (see container_roles' "songs" role).
    A failed individual future is logged and skipped, not fatal to the rest of the batch.
    Returns a future that resolves once every item has been attempted.
    Types::Album/Playlist progressive semantics (merging into an existing entry rather
    than treating each arrival as a new row) are deliberately NOT handled here — this
    stays type-agnostic, same as the rest of AbstractMediaSequence. */
    QFuture<void> progressive_batch_append (QList<QFuture<Types::Any>> futures);

    // remove items in batch using their persistent indices
    void batch_remove (const QList<QPersistentModelIndex> &items);

    std::optional<size_t> __row_pointed_to_unlocked (const QPersistentModelIndex &idx) const;

    mutable QReadWriteLock m_lock;
    
    void remove(size_t index);
    void clear();
};

#include "abstractmediasequence.tpp"