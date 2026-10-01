import QtQuick

QtObject {
    id: root

    required property Item target

    // null = window (scene) coordinates. Set it to another Item to get the
    // target's rectangle in that item's coordinates (e.g. foreground → background).
    property Item relativeTo: null

    readonly property rect bounds: _compute(target, relativeTo)

    readonly property real x: bounds.x
    readonly property real y: bounds.y
    readonly property real width: bounds.width
    readonly property real height: bounds.height

    // Reads every property that participates in the item → scene transform so
    // the binding engine subscribes to them. The sum is meaningless and is only
    // returned so the reads cannot be considered dead code.
    function _touch(item: Item): real {
        let sink = 0
        for (let it = item; it; it = it.parent)
            sink += it.x + it.y + it.width + it.height + it.scale + it.rotation
        return sink
    }

    function _compute(item: Item, ref: Item): rect {
        _touch(item)
        _touch(ref) // no-op when ref is null

        if (!item || item.width <= 0 || item.height <= 0)
            return Qt.rect(0, 0, 0, 0)

        const w = item.width
        const h = item.height

        const tl = item.mapToItem(ref, 0, 0)
        const tr = item.mapToItem(ref, w, 0)
        const bl = item.mapToItem(ref, 0, h)
        const br = item.mapToItem(ref, w, h)

        const x0 = Math.min(tl.x, tr.x, bl.x, br.x)
        const y0 = Math.min(tl.y, tr.y, bl.y, br.y)
        const xe = Math.max(tl.x, tr.x, bl.x, br.x)
        const ye = Math.max(tl.y, tr.y, bl.y, br.y)

        return Qt.rect(x0, y0, xe - x0, ye - y0)
    }
}