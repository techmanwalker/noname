import QtQuick

// Window resize handles for the corners of the Item it is declared in,
// attached the same way a DragHandler or TapHandler is:
//
//     ApplicationWindow {
//         ResizeHandles { }
//     }
//
// The window being resized is whichever one the parent Item lives in
// (Window.window), so nothing needs to be wired up by id. Intended for
// frameless windows. Set `enabled: false` to switch every handle off at once.
Item {
    id: root

    // Thickness of the grab area, in pixels
    property int handleSize: 20

    anchors.fill: parent

    z: 100 // Guarantees handles sit above all UI elements

    component ResizeHandle: Item {
        id: handle

        property int edges
        property int cursorShape

        HoverHandler {
            cursorShape: handle.cursorShape
        }

        DragHandler {
            // The handle itself must never move: the compositor owns the gesture
            // once startSystemResize() is called.
            target: null

            onActiveChanged: {
                const window = handle.Window.window;
                if (active && window) {
                    window.startSystemResize(handle.edges);
                }
            }
        }
    }

    // Top, Bottom, Left, Right edges
    /*
    ResizeHandle { anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.leftMargin: root.handleSize; anchors.rightMargin: root.handleSize; height: root.handleSize; edges: Qt.TopEdge; cursorShape: Qt.SizeVerCursor }
    ResizeHandle { anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right; anchors.leftMargin: root.handleSize; anchors.rightMargin: root.handleSize; height: root.handleSize; edges: Qt.BottomEdge; cursorShape: Qt.SizeVerCursor }
    ResizeHandle { anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom; anchors.topMargin: root.handleSize; anchors.bottomMargin: root.handleSize; width: root.handleSize; edges: Qt.LeftEdge; cursorShape: Qt.SizeHorCursor }
    ResizeHandle { anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom; anchors.topMargin: root.handleSize; anchors.bottomMargin: root.handleSize; width: root.handleSize; edges: Qt.RightEdge; cursorShape: Qt.SizeHorCursor }
    */

    // Four corners
    ResizeHandle { anchors.top: parent.top; anchors.left: parent.left; width: root.handleSize; height: root.handleSize; edges: Qt.TopEdge | Qt.LeftEdge; cursorShape: Qt.SizeFDiagCursor }
    ResizeHandle { anchors.top: parent.top; anchors.right: parent.right; width: root.handleSize; height: root.handleSize; edges: Qt.TopEdge | Qt.RightEdge; cursorShape: Qt.SizeBDiagCursor }
    ResizeHandle { anchors.bottom: parent.bottom; anchors.left: parent.left; width: root.handleSize; height: root.handleSize; edges: Qt.BottomEdge | Qt.LeftEdge; cursorShape: Qt.SizeBDiagCursor }
    ResizeHandle { anchors.bottom: parent.bottom; anchors.right: parent.right; width: root.handleSize; height: root.handleSize; edges: Qt.BottomEdge | Qt.RightEdge; cursorShape: Qt.SizeFDiagCursor }
}
