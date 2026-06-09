#pragma once

#include "abstractmediasequence.hpp"

class PlaylistSequence : public AbstractMediaSequence {
    Q_OBJECT
    QML_ANONYMOUS
public:
    explicit PlaylistSequence(QObject *parent = nullptr);

    // only accepts songs
    Q_INVOKABLE void append(const Types::Song &song);
    Q_INVOKABLE void batch_append(const QList<Types::Song> &songs); // helper for the other batch_append
    Q_INVOKABLE void batch_append(const QList<QUrl> &sources); // only performs song metadata extraction
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void clear();

private:
};