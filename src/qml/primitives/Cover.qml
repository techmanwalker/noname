import QtQuick

Image {
    id: root

    // source: ""  // URI string or QUrl
    fillMode: Image.PreserveAspectCrop  // or PreserveAspectFit, Stretch, etc.

    height: width

    asynchronous: true
}