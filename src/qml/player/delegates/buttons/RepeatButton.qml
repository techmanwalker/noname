import QtQuick
import QtQuick.Controls

ToolButton {
    id: root

    // Definimos los estados siguiendo tu estructura
    enum RepeatMode {
        Off,
        RepeatSingle,
        RepeatAllTracks
    }

    // Estado inicial
    property int currentMode: RepeatButton.RepeatMode.Off

    // Mapeo de íconos según el estado
    function getRepeatIcon(mode) {
        switch (mode) {
            case RepeatButton.RepeatMode.RepeatSingle:    return "media-playlist-repeat-song";
            case RepeatButton.RepeatMode.RepeatAllTracks: return "media-playlist-repeat";
            case RepeatButton.RepeatMode.Off:             return "media-repeat-none";
            default:                                      return "media-repeat-none";
        }
    }

    // Asignación dinámica del ícono
    icon.name: getRepeatIcon(root.currentMode)

    // Lógica de rotación cíclica
    onClicked: {
        // (0+1)%3 = 1 -> (1+1)%3 = 2 -> (2+1)%3 = 0 ... y así infinitamente
        root.currentMode = (root.currentMode + 1) % 3;
    }
    
    // Opcional: Un ToolTip para que el usuario sepa en qué modo está
    ToolTip.visible: hovered
    ToolTip.text: {
        switch (currentMode) {
            case RepeatButton.RepeatMode.RepeatSingle:    return "Repetir una"
            case RepeatButton.RepeatMode.RepeatAllTracks: return "Repetir todo"
            default:                                      return "Repetición desactivada"
        }
    }
}