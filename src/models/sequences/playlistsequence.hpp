#pragma once

#include "abstractmediasequence.hpp"
#include <QFuture>
#include <QObject>

class PlaylistSequence : public AbstractMediaSequence {
    Q_OBJECT
    QML_ANONYMOUS
public:
    explicit PlaylistSequence(QObject *parent = nullptr);

    // copy overload
    explicit PlaylistSequence(
        QList<QUrl>    sources_to_build_from,
        QFuture<void> *loading_finished_future = nullptr,
        QObject       *parent = nullptr
    );

    explicit PlaylistSequence(
        QList<Types::Song>  songs,
        QObject            *parent = nullptr
    );

    // only accepts songs
    void append(const Types::Song &song);
    void batch_append(const QList<Types::Song> &songs); // helper for the other batch_append
    QFuture<void> batch_append(const QList<QUrl> &sources); // only performs song metadata extraction
    void items();
    template <typename media_type>
        QList<media_type> items() const {
            return AbstractMediaSequence::items<media_type>();
        }
    void remove(int index);
    void clear();

private:
};