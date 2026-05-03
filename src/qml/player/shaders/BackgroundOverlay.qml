import QtQuick

ShaderEffect {
    id: root

    // Source — connect to output of DualKawaseBlur
    property ShaderEffectSource source: null

    // ── Control points: vec4(x_normalizado, L_mult, C_mult, 0) ───────────
    // x: 0.0 = left border, 1.0 = right border (bindable to UI items)
    // L_mult: 0.0 = black, 1.0 = no brightness change
    // C_mult: 0.0 = gray, 1.0 = no saturation change

    property vector4d pointA:  Qt.vector4d(0.00, 0.15, 0.20, 0) // extreme left
    property vector4d pointPA: Qt.vector4d(0.20, 0.55, 0.60, 0) // left player border
    property vector4d pointCA: Qt.vector4d(0.35, 0.85, 0.90, 0) // left cover border
    property vector4d pointCB: Qt.vector4d(0.65, 0.85, 0.90, 0) // right cover border
    property vector4d pointPB: Qt.vector4d(0.80, 0.55, 0.60, 0) // right player border
    property vector4d pointB:  Qt.vector4d(1.00, 0.15, 0.20, 0) // extreme right

    fragmentShader: "qrc:/shaders/background_overlay.frag.qsb"
}