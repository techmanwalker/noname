pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

import Player.Browser

Column {
    id: root

    property StackLayout stack

    // name -> stack index, one entry per VTabPage in the stack
    readonly property var indices: {
        const map = {};
        const pages = root.stack ? root.stack.children : [];
        for (let i = 0; i < pages.length; ++i) {
            const page = pages[i] as VTabPage;
            if (page)
                map[page.name] = i;
        }
        return Object.freeze(map);
    }

    Repeater {
        model: root.stack ? root.stack.children : []

        VTabButton {
            required property VTabPage modelData
            required property int index

            iconName: modelData.iconName
            text: modelData.title

            onClicked: root.stack.currentIndex = index
        }
    }
}