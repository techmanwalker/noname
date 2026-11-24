import QtQuick

// A page and its navbar projection, travelling together as one unit along the strip of
// its FrameStack. It fills the stack and sits `slot` pages away from the one in view.
Item {
    id: root

    required property int slot

    // a Frame is meaningless outside a FrameStack
    readonly property FrameStack stack: parent as FrameStack

    readonly property real offset: slot - stack.slide

    width: stack.width
    height: stack.height
    y: offset * height

    // only the frames the window is showing, fully or partly
    visible: Math.abs(offset) < 1
}
