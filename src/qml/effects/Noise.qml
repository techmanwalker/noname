import QtQuick

Item {
    id: root

    // Bidirectional public interface for free chaining and reorder
    property var   source: null  // input item: Item, ShaderEffectSource or variants
    property alias outputSource: _outputSource

    // Parámetros de personalización del grano
    // Grain customization
    property real  intensity: 0.04       // noise strength
    property int   seedSize:  64         // seed noise texture dimensions

    // Immediate proxy to force the input to be treated strictly as a texture
    ShaderEffectSource {
        id: _proxySource
        sourceItem: root.source
        hideSource: true
        visible: false
        width: root.width
        height: root.height
    }

    // Generate the seed on GPU
    ShaderEffect {
        id: _seedEffect
        width: root.seedSize
        height: root.seedSize
        visible: false

        property real seedTime: Math.random() * 100.0 // unique random seed by instance
        fragmentShader: "qrc:/shaders/noise_seed.frag.qsb"
    }

    // Seed storage: Optimal frozen single channel R8 format
    ShaderEffectSource {
        id: _noiseTextureProvider
        sourceItem: _seedEffect
        width: root.seedSize
        height: root.seedSize
        hideSource: false
        live: false                         // render once and freeze
        wrapMode: ShaderEffectSource.Repeat // allows for infinite repeat without breaking borders
        format: ShaderEffectSource.Alpha    // reduce memory usage to 1/4
        visible: false
    }

    // GPU effect processing
    ShaderEffect {
        id: _effect
        anchors.fill: parent
        visible: false

        // uniforms linked to the 'buf' block of the shader
        property size  nSize:     Qt.size(root.seedSize, root.seedSize)
        property real  intensity: root.intensity
        property real  cellSeed:  Math.random() * 1000.0 // reroll tile transforms per instance

        // texture injection to the corresponding bindings
        property var sourceTexture: _proxySource
        property var noiseTexture: _noiseTextureProvider

        fragmentShader: "qrc:/shaders/noise.frag.qsb"
    }

    // output node exposed to the interface
    ShaderEffectSource {
        id: _outputSource
        sourceItem: _effect
        width:  root.width
        height: root.height
        hideSource: true
        live: true
    }
}