import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ColumnLayout {
    ColumnLayout {
        id: firstHalf
        Layout.alignment: Qt.AlignTop | Qt.AlignHCenter

        Button {
            id: showPlaylist
            icon.name: "view-media-playlist"

            flat: true
        }
    }

    ColumnLayout {
        id: secondHalf
        Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter

        VolumeControl {
            direction: FlexboxLayout.Column
        }
    }
}