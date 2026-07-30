import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

Row {
    id: root

    property string messageWhenUnticked
    property string messageWhenTicked

    property string untickedIconName
    property string tickedIconName

    property bool magnify

    property real iconSize: magnify ? 24 : 18

    property bool isTicked

    leftPadding: 24
    rightPadding: 16
    topPadding: 8
    bottomPadding: 8
    spacing: magnify ? 12 : 8

    signal clicked()
    signal ticked()
    signal unticked()

    TapHandler {
        onTapped: {
            root.isTicked = !root.isTicked;
            root.isTicked ? root.ticked() : root.unticked();
            root.clicked();
        }
    }

    IconLabel {
        icon.name: root.isTicked ? root.tickedIconName : root.untickedIconName
        
        icon.width: root.iconSize
        icon.height: root.iconSize

        anchors.verticalCenter: parent.verticalCenter
    }

    MediumLabel {
        text: root.isTicked ? root.messageWhenTicked : root.messageWhenUnticked
        visible: text.length > 0

        magnify: root.magnify
    }
}