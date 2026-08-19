import QtQuick

Rectangle {
    id: root

    // to style hoverable elements too
    property bool hovered // bind to "hovered" property if needed
    property bool hoverEnabled // assign to true if the background is intended for a hoverable element

    property bool down // bind to down property, this is the "visually clicked" indicator
    property bool clickEnabled // bind to the button "enabled" property
    property bool clickable // set true for clickable elements like buttons

    property bool filled: false

    property real clickedOpacity: filled ? 1 : .7
    property real hoveredOpacity: filled ? 1 : .3
    property real defaultOpacity: filled ? 1 : ((hoverEnabled || clickable)? 0 : .3)

    property color baseColor: filled ? "#dfdfdf" : "#242424"
    property color hoveredColor: filled ? "#afafaf" : "#242424"
    property color clickedColor: "#969696"

    color: baseColor

    opacity: defaultOpacity

    radius: root.filled ? (height / 2) : 0 // perfect round

    states: [
        State {
            name: "clicked"

            PropertyChanges {
                root.color: root.clickedColor
                root.opacity: root.clickedOpacity
            }

            when: root.clickable && root.clickEnabled && root.down
        },

        State {
            name: "hovered"

            PropertyChanges {
                root.color: root.hoveredColor
                root.opacity: root.hoveredOpacity
            }

            when: root.hoverEnabled && root.hovered
        }
    ]
}