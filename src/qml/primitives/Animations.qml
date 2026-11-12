pragma Singleton
import QtQuick

// All expressed in milliseconds.

QtObject {
    component ScrollDuration: QtObject {
        required final property int transition
        required final property int cooldown
    }

    component ScrollDurations: QtObject {
        required final property ScrollDuration lyrics
        required final property ScrollDuration playlist
    }

    component DraggingTimes: QtObject {
        required final property int threshold
        required final property int release
    }

    final readonly property ScrollDurations scroll: ScrollDurations {
        lyrics: ScrollDuration {
            transition: 125
            cooldown: 5000
        }
        playlist: ScrollDuration {
            transition: 125
            cooldown: 10000
        }
    }

    final readonly property DraggingTimes dragging: DraggingTimes {
        threshold: 300
        release: 250
    }
}
