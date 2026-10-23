#pragma once

#include <QObject>
#include <QUrl>
#include <QtTypes>

#include "interfaceid.hpp"
#include "mediatypes.hpp" 

namespace Types {
    struct Song;
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

    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void unload() = 0;
    virtual void set_position(const quint64 position_ms) = 0;
    virtual void relative_seek (qint64 step_ms) = 0; // + advances, - goes back
    virtual void set_volume(quint8 volume_percent) = 0;
    virtual void load(const QUrl &source) = 0;                // designates A
    virtual void prepare_next_track(const QUrl &source) = 0;  // designates B
    virtual void undo_prepare_next_track() = 0;               // drops B

    // no Types::Song below this point; the engine only sees sources
    void load(const Types::Song &song)
    {
        if (!song.is_valid()) return;
        load(song.source);
    }
    void prepare_next_track(const Types::Song &song) { prepare_next_track(song.source); }

    virtual const QUrl & current_track_source()       const = 0;   // A
    virtual const QUrl & next_track_prepared_source() const = 0;   // B
    virtual quint64     current_position_ms() const = 0;
    virtual quint8      current_volume()      const = 0;
    virtual playback_state get_playback_state() const = 0;
    virtual bool is_a_song_loaded() const = 0;
};

inline constexpr const char* audio_engine_iid = interface_iid<"audio_engine">.buf;
Q_DECLARE_INTERFACE(audio_engine, audio_engine_iid)