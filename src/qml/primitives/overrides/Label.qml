import QtQuick
import QtQuick.Templates as T

T.Label {
    id: control

    renderType: Text.QtRendering
    renderTypeQuality: Text.VeryHighRenderTypeQuality

    color: control.palette.windowText
    linkColor: control.palette.link
}