#pragma once

#include <QFuture>

class LyricsProjector {

public:
    virtual ~LyricsProjector () = default;

    virtual QFuture<void> repopulate_with_lyrics_for_file(const QString &source) = 0;

    virtual void clear () = 0;

};

Q_DECLARE_INTERFACE(LyricsProjector, "com.noname.LyricsProjector");