import QtQuick
import QtQuick.Layouts

import Player.App
import Player.Browser
import Player.Theater
import Player.Presenter
import Player.Primitives

// One projection of the window navbar. Main instantiates one per page, as a child of
// that page's frame, so each travels with its page during a slide. What must stay a
// single entity (the SearchBar) is declared once by Main and lands in the search area
// of whichever projection it is declared in.
RowLayout {
    id: root

    required property Window window
    required property Background background

    property bool backVisible: false
    property bool loading: false
    property real leadingWidth: 0

    // glyph colors follow the backdrop under this projection, instead of staying light
    property bool adaptive: false

    // fade out until hovered
    property bool autoHide: false

    signal backClicked()

    // what the page must leave free below the bar
    readonly property alias reservedHeight: windexRow.height

    default property alias searchArea: windexRow.data

    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right

    opacity: (!autoHide || navbar_hover.hovered) ? 1 : 0

    HoverHandler {
        id: navbar_hover
    }

    RowLayout {
        Layout.maximumWidth: root.leadingWidth

        ResizableButton {
            id: backButton
            iconName: "arrow-left"

            visible: root.backVisible

            Layout.alignment: Qt.AlignVCenter

            text: qsTr("Back")

            leftPadding: 20
            rightPadding: leftPadding
            topPadding: 12
            bottomPadding: topPadding

            magnify: true

            spacing: 12

            // `visible`: a hidden page's projection has nothing to sample
            lightMode: root.adaptive && root.visible
                && Light.lightModeForBounds(backButtonBounds.bounds, root.background, PlayerPresenter)

            onClicked: root.backClicked()
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            TapHandler {
                onDoubleTapped: windex.toggleMaximize_b()
            }

            DragHandler {
                target: null

                onActiveChanged: {
                    if (active) {
                        root.window.startSystemMove();
                    }
                }
            }
        }

        LoadingIndicator {
            Layout.preferredWidth:  implicitWidth  * .7
            Layout.preferredHeight: implicitHeight * .7

            visible: root.loading
        }
    }

    RowLayout {
        id: windexRow

        Layout.fillWidth: true
        Layout.alignment: Qt.AlignVCenter
    }

    Item {
        id: navbarFiller
        Layout.fillWidth: true
        Layout.fillHeight: true

        TapHandler {
            onDoubleTapped: windex.toggleMaximize_b()
        }

        DragHandler {
            target: null

            onActiveChanged: {
                if (active) {
                    root.window.startSystemMove();
                }
            }
        }
    }

    Row {
        Layout.alignment: Qt.AlignVCenter

        WindowDecorations {
            id: windex
            window: root.window

            Layout.fillWidth: true

            anchors.verticalCenter: parent.verticalCenter

            lightMode: root.adaptive && root.visible
                && Light.lightModeForBounds(windexBounds.bounds, root.background, PlayerPresenter) // context is everything
        }
    }

    // Sampled in the page's own coordinates, i.e. where the bar sits once the page has
    // settled, so the glyph color is stable for the whole slide.
    SceneRect {
        id: windexBounds
        target: windex
        relativeTo: root.parent
    }

    SceneRect {
        id: backButtonBounds
        target: backButton
        relativeTo: root.parent
    }
}
