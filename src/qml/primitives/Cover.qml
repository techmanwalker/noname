import QtQuick

Image {
    id: root

    // source: ""  // URI string or QUrl
    fillMode: Image.PreserveAspectCrop  // or PreserveAspectFit, Stretch, etc.

    height: width

    asynchronous: true

    sourceSize: Qt.size(width, height)
    
    // Smooth scaling for better quality
    smooth: true

    mipmap: true
}