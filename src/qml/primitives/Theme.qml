pragma Singleton
import QtQuick

QtObject {
    component TextPalette: QtObject {
        required final property color primaryText
        required final property color secondaryText
        required final property color tertiaryText
        required final property color unhighlightedLyric
        required final property color highlightedLyric
    }

    component SliderPalette: QtObject {
        required final property color accentColor
        required final property color backgroundColor
    }

    component ScrollbarPalette: QtObject {
        required final property color accentColor
        required final property color backgroundColor
    }

    component ElementBackgroundPalette: QtObject {
        required final property color base
        required final property color baseFilled
        required final property color hovered
        required final property color hoveredFilled
        required final property color clicked

        required final property color songPlaying
        required final property color songSelected
        required final property color songPlayingSelected
        required final property color songHovered

        required final property color rejectedDrop

        required final property color folder
    }

    component Palette: QtObject {
        required final property TextPalette texts
        required final property SliderPalette slider
        required final property ScrollbarPalette scrollbar
        required final property ElementBackgroundPalette elementbg
        required final property color iconColor
    }

    component ItemSizes: QtObject {
        required final property int cardCoverWidth
        required final property int cardCoverHeight

        required final property int rowTrackCoverWidth
        required final property int rowTrackCoverHeight
        required final property int rowTrackFadePadding
        required final property int rowTrackLateralPadding

        required final property int scrollBarWidth
    }

    final readonly property Palette dark: Palette {
        texts: TextPalette {
            primaryText: "white"
            secondaryText: "#dfdfdf"
            tertiaryText: "#cfcfcf"
            unhighlightedLyric: "#80ffffff"
            highlightedLyric: "white"
        }
        slider: SliderPalette {
            accentColor: "white"
            backgroundColor: "#40ffffff"
        }
        scrollbar: ScrollbarPalette {
            accentColor: "white"
            backgroundColor: "#40ffffff"
        }
        elementbg: ElementBackgroundPalette {
            base: "#242424"
            baseFilled: "#dfdfdf"
            hovered: "#242424"
            hoveredFilled: "#afafaf"
            clicked: "#969696"

            songPlaying:         "#2faeaeae"
            songSelected:        "#6ea0a0a0"
            songPlayingSelected: "#74c4c4c4"
            songHovered:         "#14a0a0a0"

            rejectedDrop:   "#8ce62626"

            folder: "#1f1f1f"
        }
        iconColor: "black"
    }

    final readonly property Palette light: Palette {
        texts: TextPalette {
            primaryText: "black"
            secondaryText: "#080808"
            tertiaryText: "#121212"
            unhighlightedLyric: "#80000000"
            highlightedLyric: "black"
        }
        slider: SliderPalette {
            accentColor: "black"
            backgroundColor: "#80000000"
        }
        scrollbar: ScrollbarPalette {
            accentColor: "black"
            backgroundColor: "#40000000"
        }
        elementbg: ElementBackgroundPalette {
            base: "#242424"
            baseFilled: "#dfdfdf"
            hovered: "#242424"
            hoveredFilled: "#afafaf"
            clicked: "#969696"

            songPlaying:         "#2f222222"
            songSelected:        "#6e232323"
            songPlayingSelected: "#78181818"
            songHovered:         "#14232323"

            rejectedDrop:   "#8ce62626"

            folder: "#a0a0a0"
        }
        iconColor: "white"
    }

    final readonly property ItemSizes sizes: ItemSizes {
        cardCoverWidth:   144
        cardCoverHeight:  cardCoverWidth

        rowTrackCoverWidth:     48
        rowTrackCoverHeight:    rowTrackCoverWidth
        rowTrackFadePadding:    10
        rowTrackLateralPadding: 15

        scrollBarWidth: 4
    }
}
