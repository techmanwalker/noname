#pragma once

#include <QFuture>

class LyricsManifest
{

public:
    virtual ~LyricsManifest () = default;

    virtual QFuture<void> repopulate_with_lyrics_for_file(const QString &source) = 0;

    virtual void clear () = 0;

    virtual QModelIndex index_of_first_highlighted_row () const = 0;

    virtual std::optional<quint64> ts_of_lyric_at(quint64 ts_ms) const = 0;      // 0 if ts_ms precedes the first line
    virtual std::optional<quint64> next_lyric_ts_at(quint64 ts_ms) const = 0;    // 0 if there's no line after the active one
    virtual std::optional<QString> lyric_at(quint64 ts_ms) const = 0;            // empty if ts_ms precedes the first line

};

Q_DECLARE_INTERFACE(LyricsManifest, "com.noname.LyricsManifest");