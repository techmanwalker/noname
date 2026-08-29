import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T

import Player.Primitives

T.TextField {
    id: control

    background: ElementBackground {
        anchors.fill: parent
        anchors.centerIn: parent

        radius: height / 2 // perfect round
    }

    implicitWidth: implicitBackgroundWidth + leftInset + rightInset
                   || Math.max(contentWidth, placeholder.implicitWidth) + leftPadding + rightPadding
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding,
                             placeholder.implicitHeight + topPadding + bottomPadding)

    height: font.pointSize * 4
    width: font.pointSize * 32

    leftPadding: height / 2 // match the radius
    rightPadding: leftPadding

    placeholderText: qsTr("Search…")
    placeholderTextColor: "#afafaf"

    color: "white"
    selectionColor: control.palette.highlight
    selectedTextColor: control.palette.highlightedText
    verticalAlignment: TextInput.AlignVCenter

    PlaceholderText {
        id: placeholder
        x: control.leftPadding
        y: control.topPadding
        width: control.width - (control.leftPadding + control.rightPadding)
        height: control.height - (control.topPadding + control.bottomPadding)

        text: control.placeholderText
        font: control.font
        color: control.placeholderTextColor
        verticalAlignment: control.verticalAlignment
        visible: !control.length && !control.preeditText && (!control.activeFocus || control.horizontalAlignment !== Qt.AlignHCenter)
        elide: Text.ElideRight
        renderType: control.renderType
    }

    font.pointSize: 12
    font.weight: Font.Medium
}