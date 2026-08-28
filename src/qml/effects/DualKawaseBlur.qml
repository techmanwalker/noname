import QtQuick

Item {
    id: root

    // Bidirectional public interface for free chaining and reorder
    property var   source: null   // input item: Item, ShaderEffectSource or variants
    property int   passes: 3      // N downsamples + N upsamples
    property real  offset: 1.0    // offset by pass (in texels)
    property bool  frozen: false  // true → freeze the last frame (no recalculation)

    property alias outputSource: _outputSource

    // Hard-locked allocation bounds
    property real maximumWidth: Screen.width
    property real maximumHeight: Screen.height

    // internal state
    property var _chain: []

    // Hidden container for all intermediate passes
    Item {
        id: _passContainer
        visible: false
        width:  root.width
        height: root.height
    }

    // Final output captures last ShaderEffect of the chain
    ShaderEffectSource {
        id: _outputSource
        sourceItem: null
        width:  root.width
        height: root.height
        live:   !root.frozen
        hideSource: true
    }

    // Templates

    Component {
        id: _srcTemplate
        ShaderEffectSource { hideSource: true }
    }

    Component {
        id: _downTemplate
        ShaderEffect {
            fragmentShader: "qrc:/shaders/kawase_down.frag.qsb"
            property var      source:    null
            property vector2d texelSize: Qt.vector2d(0, 0)
            property real     offset:    1.0
        }
    }

    Component {
        id: _upTemplate
        ShaderEffect {
            fragmentShader: "qrc:/shaders/kawase_up.frag.qsb"
            property var      source:    null
            property vector2d texelSize: Qt.vector2d(0, 0)
            property real     offset:    1.0
        }
    }

    // Chain builder
    function _rebuild() {
        console.log("rebuild called.")
        // Destroy last chain
        for (var i = 0; i < _chain.length; i++)
            _chain[i].destroy()
        _chain = []
        _outputSource.sourceItem = null

        if (!root.source || root.passes <= 0 || root.width <= 0 || root.height <= 0)
            return

        let items    = []
        let prevItem = root.source
        let maxW     = root.maximumWidth
        let maxH     = root.maximumHeight

        // N downsample passes
        for (let d = 0; d < root.passes; d++) {
            maxW = Math.max(1, Math.floor(maxW / 2))
            maxH = Math.max(1, Math.floor(maxH / 2))
            
            let passDivisor = Math.pow(2, d + 1)

            let src = _srcTemplate.createObject(_passContainer, {
                sourceItem: prevItem,
                textureSize: Qt.size(maxW, maxH) // Hard-lock allocated FBO size
            })
            
            // Dynamically bind active bounds inside the locked buffer
            src.live = Qt.binding(() => !root.frozen)
            src.width = Qt.binding(() => Math.max(1, Math.floor(root.width / passDivisor)))
            src.height = Qt.binding(() => Math.max(1, Math.floor(root.height / passDivisor)))

            let effect = _downTemplate.createObject(_passContainer, {
                source: src
            })
            effect.width = Qt.binding(() => src.width)
            effect.height = Qt.binding(() => src.height)
            effect.texelSize = Qt.binding(() => Qt.vector2d(1.0 / src.width, 1.0 / src.height))
            effect.offset = Qt.binding(() => root.offset)

            items.push(src, effect)
            prevItem = effect
        }

        // N upsample passes
        for (let u = 0; u < root.passes; u++) {
            let divForSrc = Math.pow(2, root.passes - u)
            let divForEffect = Math.pow(2, root.passes - u - 1)
            
            let srcMaxW = maxW
            let srcMaxH = maxH
            maxW = Math.min(root.maximumWidth, maxW * 2)
            maxH = Math.min(root.maximumHeight, maxH * 2)

            let usrc = _srcTemplate.createObject(_passContainer, {
                sourceItem: prevItem,
                textureSize: Qt.size(srcMaxW, srcMaxH) // Hard-lock allocated FBO size
            })
            
            usrc.live = Qt.binding(() => !root.frozen)
            usrc.width = Qt.binding(() => Math.max(1, Math.floor(root.width / divForSrc)))
            usrc.height = Qt.binding(() => Math.max(1, Math.floor(root.height / divForSrc)))

            let ueffect = _upTemplate.createObject(_passContainer, {
                source: usrc
            })
            ueffect.width = Qt.binding(() => Math.min(root.width, Math.floor(root.width / divForEffect)))
            ueffect.height = Qt.binding(() => Math.min(root.height, Math.floor(root.height / divForEffect)))
            ueffect.texelSize = Qt.binding(() => Qt.vector2d(1.0 / usrc.width, 1.0 / usrc.height))
            ueffect.offset = Qt.binding(() => root.offset)

            items.push(usrc, ueffect)
            prevItem = ueffect
        }

        _outputSource.sourceItem = prevItem
        _chain = items
    }

    // Triggers
    Component.onCompleted: _rebuild()
    onSourceChanged:       _rebuild()
    onPassesChanged:       _rebuild()
}