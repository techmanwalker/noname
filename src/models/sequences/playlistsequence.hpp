#pragma once

#include "abstractmediasequence.hpp"
#include <QFuture>
#include <QObject>

class PlaylistSequence : public AbstractMediaSequence {
    Q_OBJECT
    QML_ANONYMOUS
public:
    explicit PlaylistSequence(QObject *parent = nullptr);

    explicit PlaylistSequence(
        QList<QUrl>    sources_to_build_from,
        QFuture<void> *loading_finished_future = nullptr,
        QObject       *parent = nullptr
    );

    // only accepts songs
    Q_INVOKABLE void append(const Types::Song &song);
    Q_INVOKABLE void batch_append(const QList<Types::Song> &songs); // helper for the other batch_append
    Q_INVOKABLE QFuture<void> batch_append(const QList<QUrl> &sources); // only performs song metadata extraction
    Q_INVOKABLE void items();
    Q_INVOKABLE template <typename media_type>
                QList<media_type> items() const {
                    return AbstractMediaSequence::items<media_type>();
                }
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void clear();

private:
};