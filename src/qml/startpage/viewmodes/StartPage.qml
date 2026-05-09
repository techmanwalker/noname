import QtQuick
import QtQuick.Controls

import StartPage

Item {
    id: root

    Shortcuts {     
        model: ShortcutsList
    }
}