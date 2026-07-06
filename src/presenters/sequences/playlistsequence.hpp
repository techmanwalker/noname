#pragma once

#include "abstractmediasequence.hpp"

#include <QObject>

class PlaylistSequence : public AbstractMediaSequence {
    Q_OBJECT
    QML_ANONYMOUS
public:
    explicit PlaylistSequence(QObject *parent = nullptr);

    explicit PlaylistSequence(
        QList<Types::Song>  songs,
        QObject            *parent = nullptr
    );

    // only accepts songs
    void append(const Types::Song &song);
    void batch_append(const QList<Types::Song> &songs); // helper for the other batch_append
    void items();
    template <typename media_type>
        QList<media_type> items() const {
            return AbstractMediaSequence::items<media_type>();
        }
    void remove(size_t index);
    void clear();

    /// clear and repopulate this playlist in one step
    void          respawn_list (const QList<Types::Song> &new_list);

private:
};
