pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts

import Player.App
import Player.Browser
import Player.Theater
import Player.Presenter
import Player.Primitives

T.ApplicationWindow {
    id: root

    visible: true
    title: "noname"
    color: "#000"

    flags: Qt.Window | Qt.FramelessWindowHint

    width: WindowGeometry.width
    height: WindowGeometry.height

    onWidthChanged:  {
        WindowGeometry.width  = width;
        Light.invalidate()
    }
    onHeightChanged: {
        WindowGeometry.height = height;
        Light.invalidate()
    }

    onClosing: {
        // save last used values
        WindowGeometry.poll_and_save_to_disk(width, height);
        PlayerPresenter.saveVolume();
    }

    readonly property bool typeToSearchAwaiting:
        activeView.currentIndex === 0 && // in browser
        searchBox.enabled &&             // can type
        !searchBox.activeFocus           // not focused

    readonly property bool typeToSearchFocused:
        activeView.currentIndex === 0 &&
        searchBox.activeFocus && // focused
        searchBox.clearable // enabled and not empty

    // Window-wide key routing. Events bubble from the focused item up to
    // contentItem, so this only sees keys nothing below consumed.
    function routeKey(event) {
        // type-to-search: only while the bar can actually take focus (it is
        // disabled in the player view, where focusing it would swallow the key)
        if (root.typeToSearchAwaiting) {
            const text = Keyboard.searchText(event);
            if (text !== "") {
                searchBox.feed(text);
                event.accepted = true;
            }
        }
    }

    Shortcut {
        id: clearSearchShortcut
        sequences: Keyboard.bindings.clearSearch
        enabled: searchBox.activeFocus && searchBox.clearable && !browser.isDraggingTracks
        onActivated: searchBox.clearQuery()
    }

    // Focus must land on contentItem, not nowhere: routeKey only receives
    // events when something in the window has focus.
    Shortcut {
        id: leaveSearchShortcut
        sequences: Keyboard.bindings.leaveSearch
        enabled: searchBox.activeFocus && !searchBox.clearable && !browser.isDraggingTracks
        onActivated: root.contentItem.forceActiveFocus()
    }

    // Transport keys. A Shortcut outranks a focused button or slider (only text
    // inputs and modal popups keep their own keys), so these work wherever the
    // user last clicked.
    readonly property bool transportKeys: !searchBox.activeFocus

    readonly property bool theaterMediaControls:
        activeView.currentIndex === 1 // in theater mode

    Shortcut {
        sequences: Keyboard.bindings.seekBack
        enabled: root.transportKeys
        onActivated: PlayerPresenter.shortSeekBack()
    }

    Shortcut {
        sequences: Keyboard.bindings.seekForward
        enabled: root.transportKeys
        onActivated: PlayerPresenter.shortSeekForward()
    }

    Shortcut {
        sequences: Keyboard.bindings.prevTrack
        enabled: root.transportKeys
        onActivated: PlayerPresenter.prev()
    }

    Shortcut {
        sequences: Keyboard.bindings.nextTrack
        enabled: root.transportKeys
        onActivated: PlayerPresenter.next()
    }

    Shortcut {
        sequences: Keyboard.bindings.togglePlayPause
        enabled: root.transportKeys
        autoRepeat: false // holding Space would flip play/pause on every repeat
        onActivated: PlayerPresenter.togglePlayPause()
    }

    Shortcut {
        sequences: Keyboard.bindings.raiseVolume
        enabled: root.transportKeys
        onActivated: PlayerPresenter.raiseVolume()
    }

    Shortcut {
        sequences: Keyboard.bindings.lowerVolume
        enabled: root.transportKeys
        onActivated: PlayerPresenter.lowerVolume()
    }

    Shortcut {
        sequences: Keyboard.bindings.muteVolume
        enabled: root.theaterMediaControls
        autoRepeat: false
        onActivated: PlayerPresenter.toggleMute()
    }

    // when no relevant item (the grid, the search box, etc) is focused, so global key
    // negate any focus that conflicts with a global key
    readonly property bool inVtabWBackButton: (
        // vtabs with back button support
        browser.activeVtab === browser.vtabs.playlists
    ||  browser.activeVtab === browser.vtabs.folders
    )

    Shortcut {
        sequences: Keyboard.bindings.backButton
        // Esc cancels a drag of tracks instead (MediaGrid). Both Esc shortcuts above step aside
        // too: with the search bar focused, any of the three would make Esc ambiguous.
        enabled: !clearSearchShortcut.enabled && !leaveSearchShortcut.enabled && !browser.isDraggingTracks
        onActivated: root.goBack()
    }

    // what "back" does on the current page; both navbar projections and Esc call it
    function goBack() {
        if (activeView.currentIndex === 1)
            stack_player.backButtonClicked_r();
        else if (root.inVtabWBackButton)
            browser.backButtonClicked_r();
    }

    Component.onCompleted: {
        contentItem.Keys.pressed.connect(root.routeKey);
        contentItem.forceActiveFocus(); // with nothing focused, key events have no starting point
    }

    Background {
        id: bg
        source: activeView.currentIndex === 1 ? PlayerPresenter.cover : ""
        anchors.fill: parent

        maximumWidth: Screen.width
        maximumHeight: Screen.height

        contentLightness: PlayerPresenter.coverPonderedLightness /* from 0 to 1 */

        // Math.max(1, root.width) prevents zero-division errors during
        // the brief moment when the window is still being constructed
        playerLeft:   stack_player.playerGlobalBounds.x                                          / Math.max(1, root.width)
        coverLeft:    stack_player.coverGlobalBounds.x                                           / Math.max(1, root.width)
        coverRight:  (stack_player.coverGlobalBounds.x  + stack_player.coverGlobalBounds.width)  / Math.max(1, root.width)
        playerRight: (stack_player.playerGlobalBounds.x + stack_player.playerGlobalBounds.width) / Math.max(1, root.width)

        // a pinned target looks better in lyrics mode
        readonly property real lyricsMode_cover_back_l: implicitMultiplier < 1 ? 0.5 : 0.9

        Binding on cover_back_l {
            value: bg.lyricsMode_cover_back_l
            when: stack_player.playerLyrics === 1 // in lyrics view mode
        }
    }

    FrameStack {
        id: activeView

        duration: Animations.pageswitch.duration

        onCurrentIndexChanged: Light.invalidate()

        anchors.fill: parent

        // A page and its navbar projection travel together as one frame.
        Frame {
            id: browserFrame

            slot: 0

            Browser {
                id: browser
                onRequestTheaterMode: activeView.currentIndex = 1

                searchBar: searchBox

                anchors.fill: parent
                anchors.topMargin: browserNavbar.reservedHeight
            }

            Navbar {
                id: browserNavbar

                window: root
                background: bg

                backVisible: browser.aPlaylistIsOpen || browser.aDirectoryIsOpen
                loading: browser.searching
                leadingWidth: browser.vtabsColWidth - searchBox.leftPadding

                onBackClicked: root.goBack()

                // the one SearchBar, still reachable everywhere by id
                SearchBar {
                    id: searchBox

                    Layout.preferredWidth: width
                    Layout.preferredHeight: height

                    Layout.alignment: Qt.AlignVCenter

                    onTextEdited: browser.activeVtab = browser.vtabs.tracks

                    // input only on the browser page; it stays visible while sliding out
                    enabled: activeView.currentIndex === 0
                }

                ResizableButton {
                    text: qsTr("Clear")
                    onClicked: searchBox.clearQuery()

                    leftPadding: searchBox.leftPadding / 2
                    rightPadding: searchBox.rightPadding / 2

                    visible: searchBox.visible && searchBox.clearable

                    filled: true

                    magnify: false

                    Layout.preferredHeight: searchBox.height
                    Layout.alignment: Qt.AlignVCenter
                }
            }
        }

        Frame {
            id: playerFrame

            slot: 1

            Player {
                id: stack_player

                anchors.fill: parent

                background: bg

                onBackToBrowser: activeView.currentIndex = 0
            }

            Navbar {
                id: playerNavbar

                window: root
                background: bg

                adaptive: true
                autoHide: stack_player.immersive
                backVisible: true
                loading: browser.searching
                leadingWidth: browser.vtabsColWidth - searchBox.leftPadding

                onBackClicked: root.goBack()

                // same footprint as the search area, so both projections lay out identically
                Item {
                    Layout.preferredWidth: searchBox.width
                    Layout.preferredHeight: searchBox.height
                }
            }
        }
    }

    ResizeHandles {}
}
