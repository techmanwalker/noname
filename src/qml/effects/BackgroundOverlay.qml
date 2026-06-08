import QtQuick

Item {
    id: root

    // Bidirectional public interface for free chaining and reorder
    property var   source: null  // accepts Item, ShaderEffectSource or variants
    property alias outputSource: _outputSource

    // ── Control points: vec3(x_normalized, L_mult, C_mult) ───────────
    // x: 0.0 = left border, 1.0 = right border (bindable to UI items)
    // L_mult: 0.0 = black, 1.0 = no brightness change
    // C_mult: 0.0 = gray, 1.0 = no saturation change

    property vector3d pointA:  Qt.vector3d(0.00, 0.15, 0.20) // extreme left
    property vector3d pointPA: Qt.vector3d(0.20, 0.55, 0.60) // left player border
    property vector3d pointCA: Qt.vector3d(0.35, 0.85, 0.90) // left cover border
    property vector3d pointCB: Qt.vector3d(0.65, 0.85, 0.90) // right cover border
    property vector3d pointPB: Qt.vector3d(0.80, 0.55, 0.60) // right player border
    property vector3d pointB:  Qt.vector3d(1.00, 0.15, 0.20) // extreme right

    // Immediate interface to ensure input is treated as a texture
    ShaderEffectSource {
        id: _proxySource
        sourceItem: root.source
        hideSource: true
        visible: false
        width: root.width
        height: root.height
    }

    // Real effect executed by the fragment shader
    ShaderEffect {
        id: _effect
        anchors.fill: parent
        visible: false

        // Link the property expected by the shader to the input proxy
        property var source: _proxySource

        property vector3d pointA:  root.pointA
        property vector3d pointPA: root.pointPA
        property vector3d pointCA: root.pointCA
        property vector3d pointCB: root.pointCB
        property vector3d pointPB: root.pointPB
        property vector3d pointB:  root.pointB

        fragmentShader: "qrc:/shaders/background_overlay.frag.qsb"
    }

    // Output anchor exposing the final result
    ShaderEffectSource {
        id: _outputSource
        sourceItem: _effect
        width:  root.width
        height: root.height
        hideSource: true
        live: true
    }
}