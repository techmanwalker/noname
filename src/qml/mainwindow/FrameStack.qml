import QtQuick

// A vertical strip of Frames slid past the window. `currentIndex` is the logical page;
// `slide` follows it with an animation and is what the Frames position themselves by.
Item {
    id: root

    required property int duration

    property int currentIndex: 0

    // visual position, 0 = first frame fully shown, 1 = second, ...
    property real slide: currentIndex

    Behavior on slide {
        NumberAnimation { duration: root.duration; easing.type: Easing.OutCubic }
    }
}
