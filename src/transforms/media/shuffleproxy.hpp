#pragma once

#include <QAbstractProxyModel>

#include <random>
#include <vector>

namespace MediaTransforms
{

/// Flat-list proxy: presents its source in source order, or as a random
/// permutation of it. Rows are never copied; data() is read live from the source.
/// Reordering the presentation (reshuffle, move_presented) never touches the source.
class ShuffleProxy : public QAbstractProxyModel
{
    Q_OBJECT

public:
    explicit ShuffleProxy(QObject *parent = nullptr);

    void setSourceModel(QAbstractItemModel *source) override;

    bool shuffled() const { return m_shuffled; }

    /// `lead` is an index of the SOURCE model; it becomes the first row of the new shuffled
    /// order. Ignored when disabling.
    void set_shuffled(bool enabled, const QPersistentModelIndex &lead = {});

    /// New random order. Only while shuffled; persistent indices follow their items.
    void reshuffle();

    /// The item at presented row `src` ends up at presented row `dest`; the rows in between
    /// shift by one. Reorders the presentation only: the source is untouched.
    /// False if not shuffled, or either index is invalid or not this model's.
    bool move_presented(const QPersistentModelIndex &src, const QPersistentModelIndex &dest);

    QModelIndex mapToSource   (const QModelIndex &proxy)  const override;
    QModelIndex mapFromSource (const QModelIndex &source) const override;

    QModelIndex index (int row, int column, const QModelIndex &parent = {}) const override;
    QModelIndex parent (const QModelIndex &) const override { return {}; }
    int rowCount    (const QModelIndex &parent = {}) const override;
    int columnCount (const QModelIndex &parent = {}) const override;

protected:
    void resetInternalData() override;

private:
    void rebuild_order (const QPersistentModelIndex &lead = {});
    void reorder (bool enabled, const QPersistentModelIndex &lead);
    void rebuild_rank ();

    void on_source_rows_about_to_be_inserted (const QModelIndex &parent, int first, int last);
    void on_source_rows_inserted             (const QModelIndex &parent, int first, int last);
    void on_source_rows_about_to_be_removed  (const QModelIndex &parent, int first, int last);
    void on_source_rows_removed              (const QModelIndex &parent, int first, int last);
    void on_source_rows_about_to_be_moved    (const QModelIndex &source_parent, int first, int last, const QModelIndex &dest_parent, int dest_child);
    void on_source_rows_moved                (const QModelIndex &source_parent, int first, int last, const QModelIndex &dest_parent, int dest_child);
    void on_source_data_changed (const QModelIndex &top_left, const QModelIndex &bottom_right, const QList<int> &roles);

    bool m_shuffled {false};
    std::vector<int> m_order; // proxy row -> source row (empty while not shuffled)
    std::vector<int> m_rank;  // source row -> proxy row (inverse of m_order, -1 = absent)
    std::mt19937 m_rng {std::random_device{}()};
    QList<QMetaObject::Connection> m_connections;
};

}
