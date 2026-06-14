import QtQuick

Item {
    id: root

    // Free chain interface
    property var source: null
    property alias outputSource: _outputSource

    // Specify your stops and desired alphas here
    property vector2d outerLeftStop:  Qt.vector2d(0.0, 0.0)
    property vector2d innerLeftStop:  Qt.vector2d(0.2, 1.0)
    property vector2d innerRightStop: Qt.vector2d(0.8, 1.0)
    property vector2d outerRightStop: Qt.vector2d(1.0, 0.0)
    property bool switchAxis: false

    // Immediate proxy to force the input to be treated strictly as a texture
    ShaderEffectSource {
        id: _proxySource
        sourceItem: root.source
        hideSource: true
        visible: false
        width: root.width
        height: root.height
    }

    // GPU effect processing
    ShaderEffect {
        id: _effect
        anchors.fill: parent
        visible: false

        // uniforms linked to the 'buf' block of the shader
        property var sourceTexture: _proxySource
        
        property vector2d outer_leftstop: root.outerLeftStop
        property vector2d inner_leftstop: root.innerLeftStop
        property vector2d inner_rightstop: root.innerRightStop
        property vector2d outer_rightstop: root.outerRightStop
        property bool switch_axis: root.switchAxis

        fragmentShader: "qrc:/shaders/edge_fade.frag.qsb"
    }

    // output node exposed to the interface, with applied alpha
    ShaderEffectSource {
        id: _outputSource
        sourceItem: _effect
        width: root.width
        height: root.height
        hideSource: true
        live: true
    }
}