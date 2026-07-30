import QtQuick
import QtQuick.Controls

Label {
    id: root

    property bool magnify: false // true: big text, false: small text
    
    Binding on font.pointSize {
        value: 13
        when: root.magnify
    }
    
    font.weight: magnify ? Font.Light : Font.Normal // Yagami
}