import QtQuick

Item {
    id: root

    property color fill: "#fff"
    property url source: ""  // URI string or QUrl
    property int fillMode: Image.PreserveAspectCrop  // or PreserveAspectFit, Stretch, etc.

    Rectangle {
        color: root.fill
    
        anchors.fill: root
        visible: image.status !== Image.Ready
    }

    Image {
        id: image
        anchors.fill: root
        fillMode: root.fillMode
        source: root.source
        
        // Smooth scaling for better quality
        smooth: true
        mipmap: false
        
        // show nothing if source is empty (shows root.fill color instead)
        visible: status === Image.Ready

        // imperative mipmap toggling
        onStatusChanged: {
            if (status === Image.Ready) {
                mipmap = true;
            } else if (status === Image.Null || status === Image.Error) {
                mipmap = false;
            }
        }
    }
}