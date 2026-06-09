#pragma once

#include "abstractmediasequence.hpp"
#include <qobject.h>

class PlaylistSequence : public AbstractMediaSequence {
    Q_OBJECT
    QML_ANONYMOUS
public:
    explicit PlaylistSequence(QObject *parent = nullptr);

    explicit PlaylistSequence(QList<QUrl> sources_to_build_from, QObject *parent = nullptr);

    // only accepts songs
    Q_INVOKABLE void append(const Types::Song &song);
    Q_INVOKABLE void batch_append(const QList<Types::Song> &songs); // helper for the other batch_append
    Q_INVOKABLE void batch_append(const QList<QUrl> &sources); // only performs song metadata extraction
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void clear();

private:
};