pragma Singleton
import QtQuick

QtObject {
    component TextPalette: QtObject {
        required property color primaryText
        required property color secondaryText
        required property color tertiaryText
        required property color unhighlightedLyric
        required property color highlightedLyric
    }

    component SliderPalette: QtObject {
        required property color accentColor
        required property color backgroundColor
    }

    component ScrollbarPalette: QtObject {
        required property color accentColor
        required property color backgroundColor
    }

    component ElementBackgroundPalette: QtObject {
        required property color base
        required property color baseFilled
        required property color hovered
        required property color hoveredFilled
        required property color clicked

        required property color songPlaying
        required property color songSelected
        required property color songHovered

        required property color folder
    }

    component Palette: QtObject {
        required property TextPalette texts
        required property SliderPalette slider
        required property ScrollbarPalette scrollbar
        required property ElementBackgroundPalette elementbg
        required property color iconColor
    }

    readonly property Palette dark: Palette {
        texts: TextPalette {
            primaryText: "white"
            secondaryText: "#dfdfdf"
            tertiaryText: "#afafaf"
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

            songPlaying:  ({ r: 200 / 255, g: 200 / 255, b: 200 / 255, a: .08 })
            songSelected: ({ r: 160 / 255, g: 160 / 255, b: 160 / 255, a: .3 })
            songHovered:  ({ r: 160 / 255, g: 160 / 255, b: 160 / 255, a: .08 })

            folder: "#1f1f1f"
        }
        iconColor: "black"
    }

    readonly property Palette light: Palette {
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

            songPlaying:  ({ r: 20 / 255, g: 20 / 255, b: 20 / 255, a: .08 })
            songSelected: ({ r: 35 / 255, g: 35 / 255, b: 35 / 255, a: .3 })
            songHovered:  ({ r: 35 / 255, g: 35 / 255, b: 35 / 255, a: .08 })

            folder: "#a0a0a0"
        }
        iconColor: "white"
    }
}