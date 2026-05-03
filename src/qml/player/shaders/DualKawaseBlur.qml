import QtQuick

Item {
    id: root

    // ── Public API ────────────────────────────────────────────────────────
    property Item  source: null   // input item
    property int   passes: 3      // N downsamples + N upsamples
    property real  offset: 1.0    // offset by pass (in texels)
    property bool  frozen: false  // true → freeze the last frame (no recalculation)

    // ── Internal state ─────────────────────────────────────────────────────
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

    // Display visible — passthrough without shader custom (Qt built-in)
    ShaderEffect {
        anchors.fill: parent
        property var source: _outputSource
    }

    // ── Templates ─────────────────────────────────────────────────────────

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

    // ── Chain builder ──────────────────────────────────────────────

    function _rebuild() {
        // Destroy last chain
        for (var i = 0; i < _chain.length; i++)
            _chain[i].destroy()
        _chain = []
        _outputSource.sourceItem = null

        if (!root.source || root.passes <= 0 || root.width <= 0 || root.height <= 0)
            return

        var items    = []
        var prevItem = root.source
        var w = root.width
        var h = root.height

        // ── N downsample passes ────────────────────────────────────────
        for (var d = 0; d < root.passes; d++) {
            w = Math.max(1, Math.floor(w / 2))
            h = Math.max(1, Math.floor(h / 2))

            var src = _srcTemplate.createObject(_passContainer, {
                sourceItem: prevItem,
                width:  w,
                height: h
            })
            src.live = Qt.binding(function() { return !root.frozen })

            var effect = _downTemplate.createObject(_passContainer, {
                width:     w,
                height:    h,
                source:    src,
                texelSize: Qt.vector2d(1.0 / w, 1.0 / h),
                offset:    root.offset
            })

            items.push(src, effect)
            prevItem = effect
        }

        // ── N upsample passes ──────────────────────────────────────────
        for (var u = 0; u < root.passes; u++) {
            var srcW = w
            var srcH = h
            w = Math.min(root.width,  w * 2)
            h = Math.min(root.height, h * 2)

            var usrc = _srcTemplate.createObject(_passContainer, {
                sourceItem: prevItem,
                width:  srcW,
                height: srcH
            })
            usrc.live = Qt.binding(function() { return !root.frozen })

            var ueffect = _upTemplate.createObject(_passContainer, {
                width:     w,
                height:    h,
                source:    usrc,
                texelSize: Qt.vector2d(1.0 / srcW, 1.0 / srcH),
                offset:    root.offset
            })

            items.push(usrc, ueffect)
            prevItem = ueffect
        }

        _outputSource.sourceItem = prevItem
        _chain = items
    }

    // ── Triggers ───────────────────────────────────────────────────────────
    Component.onCompleted: _rebuild()
    onSourceChanged:       _rebuild()
    onPassesChanged:       _rebuild()
    onOffsetChanged:       _rebuild()
    onWidthChanged:        _rebuild()
    onHeightChanged:       _rebuild()
}