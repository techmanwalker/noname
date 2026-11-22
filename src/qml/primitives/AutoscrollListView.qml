pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import Player.Primitives

// A ListView that follows one row (the "focus" row): whenever focusIndex
// changes it scrolls that row to a fixed spot, unless the user is driving the
// view. Autoscroll stays suspended while they interact and for a cooldown
// after they let go; a change that happened meanwhile is applied once the
// cooldown ends.
//
// Empty space before the first row / after the last one (header, footer) is
// the owner's business: the destination is always resolved by Qt, so it is
// clamped when there is no such space and reaches the exact spot when there is.
ListView {
    id: root

    // Row to follow. -1 means "no row": rest at the very beginning of the
    // content, header included.
    property int focusIndex: -1

    // Where the focus row lands. Any ListView.PositionMode that
    // positionViewAtIndex() understands.
    property int focusPosition: ListView.Center

    // Gap between the viewport's edge and the focus row, as a fraction of that row's own
    // height: from the top edge with ListView.Beginning, from the bottom edge with ListView.End.
    property real focusMarginRatio: 0

    // Duration and curve of the scroll transition; owners animating alongside it reuse them
    property int autoscrollDurationMs: Animations.scroll.lyrics.transition
    property int autoscrollEasing: Easing.InOutCubic

    // Idle time after the last user interaction before autoscroll resumes
    property int autoscrollCooldownMs: Animations.scroll.lyrics.cooldown

    property bool autoscrollEnabled: true

    // While the focus changes faster than the scroll transition, don't chase it: the running
    // transition finishes, the focus walks the rows in view, and the view scrolls only when it
    // leaves them. Once the focus has been still for a full transition, it goes back to its spot.
    property bool autoscrollPaged: false

    // The focus is running ahead of the view (paged following)
    property bool racing: false

    // true while the user drives the view (flick, wheel, drag or scrollbar)
    readonly property bool userInteracting: moving || (root.ScrollBar.vertical?.pressed ?? false)

    readonly property bool autoscrollAllowed:
        autoscrollEnabled && !userInteracting && !cooldownTimer.running

    // Scrolls to the focus row regardless of cooldown and interaction. For
    // programmatic jumps (seek, click) that are not autoscroll events.
    function scrollToFocus() : void {
        scrollToFocusAt(focusPosition)
    }

    function scrollToFocusAt(position : int) : void {
        if (focusIndex >= count)
            return

        racing = false

        const fromY = contentY

        // No focus row: rest at the very top, so whatever the header holds is
        // what's on screen.
        if (focusIndex < 0)
            positionViewAtBeginning()
        else
            positionViewAtIndex(focusIndex, position)

        // Qt resolved (and clamped) the destination: read it back
        let toY = contentY

        // The margin is the only thing Qt can't resolve. Next to either end
        // with no empty space left to scroll into, Qt's clamped position is
        // already the right one and the margin can't be honored. Anywhere
        // else, including ends padded by a header/footer, lift the row by
        // its own height fraction. positionViewAtIndex() instantiates the
        // target delegate, so it can be queried right after.
        if (focusMarginRatio > 0 && focusIndex >= 0 && !atYBeginning && !atYEnd) {
            const item = itemAtIndex(focusIndex)
            if (item && position === ListView.Beginning)
                toY -= focusMarginRatio * item.height
            else if (item && position === ListView.End)
                toY += focusMarginRatio * item.height
        }

        contentY = fromY

        scrollAnimation.to = toY
        scrollAnimation.restart()
    }

    // Scrolls to the focus row only if autoscroll is currently allowed
    function autoscroll() : void {
        if (autoscrollAllowed)
            scrollToFocus()
    }

    NumberAnimation {
        id: scrollAnimation
        target: root
        property: "contentY"
        duration: root.autoscrollDurationMs
        easing.type: root.autoscrollEasing

        onFinished: if (root.racing) Qt.callLater(root.settleFocus)
    }

    // Applies the scroll deferred while the user was interacting, once they
    // have been idle long enough.
    Timer {
        id: cooldownTimer
        interval: root.autoscrollCooldownMs
        onTriggered: if (root.autoscrollEnabled) root.scrollToFocus()
    }

    onUserInteractingChanged: {
        if (userInteracting) {
            // yield to the user immediately, and make sure the timer can't
            // fire a scroll in the middle of their gesture
            scrollAnimation.stop()
            cooldownTimer.stop()
        } else {
            cooldownTimer.restart()
        }
    }

    // Toggling restarts the cooldown, so the view settles (e.g. after the
    // owner adds or removes header/footer space) before it scrolls.
    onAutoscrollEnabledChanged: cooldownTimer.restart()

    // Emitted in the turn the view acts on a focus change, right before any scroll starts.
    // Owners animating alongside the scroll start theirs here, so both begin on the same frame.
    signal focusFollowed()

    function followFocus() : void {
        focusFollowed()

        if (!autoscrollPaged) {
            autoscroll()
            return
        }

        settleTimer.restart()
        if (scrollAnimation.running)
            racing = true       // let it finish; settleFocus takes over from there

        if (racing)
            settleFocus()
        else
            autoscroll()
    }

    // Whether the focus row is entirely out of the viewport
    function focusOffscreen() : bool {
        const item = itemAtIndex(focusIndex)
        return !item || item.y + item.height <= contentY || item.y >= contentY + height
    }

    // What a racing focus gets: nothing while it stays in view, a scroll when it leaves
    // it, its spot once it has been still for a full transition
    function settleFocus() : void {
        if (!racing || !autoscrollAllowed || scrollAnimation.running)
            return              // a finishing scroll calls back

        const still = !settleTimer.running
        if (!still && !focusOffscreen())
            return

        // Back at its spot, or paged to the side it left through: a focus that left upwards
        // lands at the bottom margin, so it has rows above it to walk through
        if (still)
            scrollToFocus()
        else
            scrollToFocusAt(focusIndex < indexAt(0, contentY + height / 2) ? ListView.End : focusPosition)

        racing = !still         // left the view mid-race, or back at its spot
    }

    Timer {
        id: settleTimer
        interval: root.autoscrollDurationMs
        onTriggered: root.settleFocus()
    }

    onFocusIndexChanged: Qt.callLater(followFocus)

    Component.onCompleted: Qt.callLater(autoscroll)
}
