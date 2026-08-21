#pragma once

#include <QFuture>

class LyricsManifest {

public:
    virtual ~LyricsManifest () = default;

    virtual QFuture<void> repopulate_with_lyrics_for_file(const QString &source) = 0;

    virtual void clear () = 0;

};

Q_DECLARE_INTERFACE(LyricsManifest, "com.noname.LyricsManifest");