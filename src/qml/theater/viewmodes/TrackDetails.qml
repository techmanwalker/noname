pragma ComponentBehavior: Bound
import QtQuick

import Player.MediaTypes
import Player.Primitives

Column {
    id: root
    required property song model

    required property bool lightMode

    component MetadataTag: Label {
        property string pattern   // translated, %1 stands for the value
        property string value
        textFormat: Text.StyledText
        visible: pattern.length > 0 && value.length > 0

        // StyledText parses markup and values come from tags in files, so escape them
        text: pattern.arg(value.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;"))

        color: root.lightMode ? Theme.light.texts.primaryText : Theme.dark.texts.primaryText
    }

    MetadataTag {
        //: %1 is the audio file format, e.g. FLAC
        pattern: qsTr("<b>Format:</b> %1")
        value: root.model.fileformat
    }

    MetadataTag {
        //: %1 is the music genre
        pattern: qsTr("<b>Genre:</b> %1")
        value: root.model.genre
    }

    MetadataTag {
        //: %1 is the release year
        pattern: qsTr("<b>Year:</b> %1")
        value: root.model.year > 0 ? root.model.year : ""
    }

    MetadataTag {
        //: %1 is the number of audio channels
        pattern: qsTr("<b>Channels:</b> %1")
        value: root.model.channels > 0 ? root.model.channels : ""
    }

    MetadataTag {
        //: %1 is the number of audio channels
        pattern: qsTr("<b>Sample rate:</b> %1 Hz")
        value: root.model.channels > 0 ? root.model.sample_rate : ""
    }

    MetadataTag {
        //: %1 is the bitrate in kilobits per second
        pattern: qsTr("<b>Bitrate:</b> %1 kbps")
        value: root.model.bitrate_kbps > 0 ? root.model.bitrate_kbps : ""
    }
}