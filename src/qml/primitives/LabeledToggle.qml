import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

Row {
    id: root

    property string messageWhenUnticked
    property string messageWhenTicked

    property string untickedIconName
    property string tickedIconName

    property real iconSize: 24

    property bool isTicked

    leftPadding: 24
    rightPadding: 16
    topPadding: 8
    bottomPadding: 8
    spacing: 12

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

    Label {
        text: root.isTicked ? root.messageWhenTicked : root.messageWhenUnticked
        visible: text.length > 0

        font.pointSize: 13
        font.weight: Font.Light // Yagami

        anchors.verticalCenter: parent.verticalCenter
    }
}