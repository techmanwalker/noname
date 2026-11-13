#pragma once

#include "virtual/lyricsmanifest.hpp"

#include "listproxybase.hpp"

#include <QIdentityProxyModel>
#include <QtQmlIntegration>

#include <memory>

class LyricsManifestProxy
    : public QIdentityProxyModel,
      public ListProxyBase<LyricsManifestProxy, LyricsManifest, void, &LyricsManifest::line_count>
{
    Q_OBJECT
    QML_NAMED_ELEMENT(LyricsManifest)
    QML_SINGLETON

    Q_PROPERTY(qsizetype count READ item_count NOTIFY countChanged)

public:
    explicit LyricsManifestProxy(QObject *parent = nullptr) : QIdentityProxyModel(parent) {
        forward(SIGNAL(count_changed()), SIGNAL(countChanged()));
    }

    // re-home the index: the interface returns one rooted in the source model
    Q_INVOKABLE QModelIndex indexOfLyricToHighlight() const {
        return mapFromSource(call(&LyricsManifest::index_of_lyric_to_highlight));
    }

signals:
    void countChanged();

public slots:
    void clear() { call(&LyricsManifest::clear); }
};
