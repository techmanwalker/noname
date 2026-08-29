import QtQuick

Item {
    id: root

    // Bidirectional public interface for free chaining and reorder
    property var   source: null  // accepts Item, ShaderEffectSource or variants
    property alias outputSource: _outputSource

    // Control points: vec3(x_normalized, L_mult, C_mult)
    
    // x: 0.0 = left border, 1.0 = right border (bindable to UI items)
    // L_mult: 0.0 = black, 1.0 = no brightness change
    // C_mult: 0.0 = gray, 1.0 = no saturation change

    property vector3d pointA:  ({ x: 0, y: 0.15, z: 0.2 }) // extreme left
    property vector3d pointPA: ({ x: 0.2, y: 0.55, z: 0.6 }) // left player border
    property vector3d pointCA: ({ x: 0.35, y: 0.85, z: 0.9 }) // left cover border
    property vector3d pointCB: ({ x: 0.65, y: 0.85, z: 0.9 }) // right cover border
    property vector3d pointPB: ({ x: 0.8, y: 0.55, z: 0.6 }) // right player border
    property vector3d pointB:  ({ x: 1, y: 0.15, z: 0.2 }) // extreme right

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