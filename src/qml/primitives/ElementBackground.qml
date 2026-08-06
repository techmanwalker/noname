import QtQuick

Rectangle {
    id: root

    // to style hoverable elements too
    property bool hovered // bind to "hovered" property if needed
    property bool hoverEnabled // assign to true if the background is intended for a hoverable element

    property bool down // bind to down property, this is the "visually clicked" indicator
    property bool clickEnabled // bind to the button "enabled" property
    property bool clickable // set true for clickable elements like buttons

    property real clickedOpacity: .7
    property real hoveredOpacity: .3
    property real defaultOpacity: (hoverEnabled || clickable)? 0 : .3

    color: "#242424"
    opacity: defaultOpacity

    states: [
        State {
            name: "clicked"

            PropertyChanges {
                root.color: "#969696"
                root.opacity: root.clickedOpacity
            }

            when: root.clickable && root.clickEnabled && root.down
        },

        State {
            name: "hovered"

            PropertyChanges {
                root.color: "#242424"
                root.opacity: root.hoveredOpacity
            }

            when: root.hoverEnabled && root.hovered
        }
    ]
}