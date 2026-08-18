import QtQuick

Image {
    id: root

    // source: ""  // URI string or QUrl
    fillMode: Image.PreserveAspectCrop  // or PreserveAspectFit, Stretch, etc.

    height: width

    asynchronous: true
    
    // Smooth scaling for better quality
    smooth: true
    mipmap: false

    // imperative mipmap toggling
    onStatusChanged: {
        if (status === Image.Ready) {
            mipmap = true;
        } else if (status === Image.Null || status === Image.Error) {
            mipmap = false;
        }
    }
}