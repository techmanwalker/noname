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
        required final property color songHovered

        required final property color folder
    }

    component Palette: QtObject {
        required final property TextPalette texts
        required final property SliderPalette slider
        required final property ScrollbarPalette scrollbar
        required final property ElementBackgroundPalette elementbg
        required final property color iconColor
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

            songPlaying:  "#14c8c8c8"
            songSelected: "#4da0a0a0"
            songHovered:  "#14a0a0a0"

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

            songPlaying:  "#14141414"
            songSelected: "#4d232323"
            songHovered:  "#14232323"

            folder: "#a0a0a0"
        }
        iconColor: "white"
    }
}