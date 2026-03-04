import QtQuick

Item {
    id: root

    property color fill: "#fff"
    property alias source: image.source  // URI string or QUrl
    property int fillMode: Image.PreserveAspectCrop  // or PreserveAspectFit, Stretch, etc.

    Rectangle {
        color: root.fill
    
        anchors.fill: root
    }

    Image {
        id: image
        anchors.fill: root
        fillMode: root.fillMode
        source: ""  // Empty by default, set via property
        
        // Smooth scaling for better quality
        smooth: true
        mipmap: true
        
        // Optional: show nothing if source is empty (shows root.fill color instead)
        visible: source !== "" && status === Image.Ready
    }
}