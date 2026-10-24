import QtQuick

// One page of Browser's vertical-tab stack. Declaring one inside the stack is
// all it takes to get its VTabButton (ArrangementList) and its entry in
// Browser.vtabs; the page's content is simply declared inside it.
Item {
    // Key in Browser.vtabs (browser.vtabs.<name>): stable, never translated
    required property string name

    // Button label. Must be a qsTr("literal") at the declaration site,
    // lupdate only extracts string literals
    required property string title

    property string iconName
}