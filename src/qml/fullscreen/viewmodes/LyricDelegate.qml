pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

import Player.LyricsManifest
import Player.Primitives

Label {
    id: root

    required property lyric model
    required property bool highlighted
    required property real wrapWidth

    readonly property int baseFontSize: 20
    readonly property int baseFontWeight: Font.Medium

    property string wrappedText: ""

    text: wrappedText

    font.pointSize: highlighted ? 25 : baseFontSize
    font.weight: highlighted ? Font.DemiBold : baseFontWeight

    Binding on font.bold {
        value: true
        when: root.highlighted
    }

    Binding on color {
        value: "white"
        when: root.highlighted
    }

    // Breaks are computed once, against the *unhighlighted* font only —
    // never against `highlighted`. Since the highlighted font is strictly
    // larger, replaying the same breaks at that size only ever needs more
    // width, never a different break.
    TextMetrics {
        id: wrapMetrics
        font.family: root.font.family
        font.pointSize: root.baseFontSize
        font.weight: root.baseFontWeight
    }

    function rewrap() : void {
        const source = root.model.text

        if (!source || root.wrapWidth <= 0) {
            wrappedText = source ?? ""
            return
        }

        const words = source.trim().split(/\s+/)
        const lines = []
        let current = ""

        for (let i = 0; i < words.length; ++i) {
            const candidate = current.length ? current + " " + words[i] : words[i]
            wrapMetrics.text = candidate

            if (wrapMetrics.advanceWidth > root.wrapWidth && current.length) {
                lines.push(current)
                current = words[i]
            } else {
                current = candidate
            }
        }

        if (current.length)
            lines.push(current)

        wrappedText = lines.join("\n")
    }

    onWrapWidthChanged: rewrap()
    onModelChanged: rewrap()
    Component.onCompleted: rewrap()
}