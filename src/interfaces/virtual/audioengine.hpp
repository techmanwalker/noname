#pragma once

#include <QFuture>
#include <QObject>
#include <QUrl>
#include <QtTypes>

#include "interfaceid.hpp"
#include "mediatypes.hpp"

namespace Types {
    struct Track;
}

/*  Vocabulary, shared by everything in src/audioengine:

    A         the active track: playing, or paused with its continuation samples still belonging
              to A, or stepping over the boundary.
    B         the track that plays once A has run out of samples. May be absent.
    boundary  where A's samples end and B's begin. The RT thread "hits" it.
    tape ran out   the boundary was hit with no B behind it: nothing left to play, state stopped.

    Two slots, strictly. load() designates A; prepare_next_track() / undo_prepare_next_track()
    designate and drop B; current_track_source() / next_track_prepared_source() read A and B.
    Those names stay public, everything behind them speaks A and B.

    The decoder is NOT part of this vocabulary. It reads files ahead of playback, so the file it
    has open belongs to A or to B depending on how far ahead it is. "A" and "B" always mean the
    engine's A and B; for the decoder say "A's file" / "B's file". */

// public interface for the audio_engine
class audio_engine
{

public:
    enum class playback_state {
        paused,
        playing,
        stopped
    };

    virtual ~audio_engine() = default;

    // transport
    virtual QFuture<void> play() = 0;
    virtual QFuture<void> pause() = 0;
    virtual QFuture<void> stop() = 0;

    // designates or drops A
    virtual QFuture<void> load(const QUrl &source) = 0;
    virtual QFuture<void> unload() = 0;

    // designates or drops B
    virtual QFuture<void> prepare_next_track(const QUrl &source) = 0;
    virtual QFuture<void> undo_prepare_next_track() = 0;

    // position
    virtual QFuture<void> seek_to(const quint64 position_ms) = 0;
    virtual QFuture<void> relative_seek (qint64 step_ms) = 0; // + advances, - goes back

    // volume
    virtual QFuture<void> set_volume(quint8 volume_percent) = 0;

    // no Types::Track below this point; the engine only sees sources
    QFuture<void> load (const Types::Track &song)
    {
        if (!song.is_valid()) return QtFuture::makeReadyVoidFuture();
        return load(song.source);
    }
    QFuture<void> prepare_next_track(const Types::Track &song) { return prepare_next_track(song.source); }

    virtual const QUrl & current_track_source()       const = 0;   // A
    virtual const QUrl & next_track_prepared_source() const = 0;   // B
    virtual quint64     current_position_ms() const = 0;
    virtual quint8      current_volume()      const = 0;
    virtual playback_state get_playback_state() const = 0;
    virtual bool is_a_track_loaded() const = 0;
};

inline constexpr const char* audio_engine_iid = interface_iid<"audio_engine">.buf;
Q_DECLARE_INTERFACE(audio_engine, audio_engine_iid)
