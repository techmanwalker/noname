pragma Singleton
import QtQuick

// Single home for keyboard policy. What a key *does* is wired in Main.qml;
// what counts as which kind of key lives here.
QtObject {
    // action → key sequences (QKeySequence strings). The one place to rebind.
    readonly property var bindings: ({
        clearSearch:     ["Esc"],   // bar has a query
        leaveSearch:     ["Esc"],   // bar is empty
        seekBack:        ["Left"],
        seekForward:     ["Right"],
        togglePlayPause: ["Space"],
        raiseVolume:     ["Up"],
        lowerVolume:     ["Down"]
    })

    // A chord carrying any of these is a shortcut, never text. Shift is
    // absent (capitals are text), as is Keypad (numpad digits).
    readonly property int shortcutModifiers: Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier

    // The text this key event should type into the search bar, or "" if the
    // event isn't search input.
    function searchText(event) {
        if (event.modifiers & shortcutModifiers)
            return "";

        const t = event.text;
        if (t.length !== 1)
            return "";

        const isDigit  = t >= "0" && t <= "9";
        const isLetter = t.toLowerCase() !== t.toUpperCase(); // any cased script; QML has no \p{L}
        return (isDigit || isLetter) ? t : "";
    }
}