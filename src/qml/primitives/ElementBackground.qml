import QtQuick

import Player.Primitives

Rectangle {
    id: root

    // to style hoverable elements too
    property bool hovered // bind to "hovered" property if needed
    property bool hoverEnabled // assign to true if the background is intended for a hoverable element

    property bool down // bind to down property, this is the "visually clicked" indicator
    property bool clickEnabled // bind to the button "enabled" property
    property bool clickable // set true for clickable elements like buttons

    property bool filled: false

    property bool lightMode: false

    property real clickedOpacity: filled ? 1 : .7
    property real hoveredOpacity: filled ? 1 : .4
    property real defaultOpacity: filled ? 1 : ((hoverEnabled || clickable)? 0 : .4)

    property color baseColor: root.lightMode 
        ? (filled ? Theme.light.elementbg.baseFilled : Theme.light.elementbg.base)
        : (filled ? Theme.dark.elementbg.baseFilled : Theme.dark.elementbg.base)

    property color hoveredColor: root.lightMode 
        ? (filled ? Theme.light.elementbg.hoveredFilled : Theme.light.elementbg.hovered)
        : (filled ? Theme.dark.elementbg.hoveredFilled : Theme.dark.elementbg.hovered)

    property color clickedColor: root.lightMode 
        ? Theme.light.elementbg.clicked 
        : Theme.dark.elementbg.clicked


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