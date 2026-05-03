#version 450

layout(location = 0) in  vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4  qt_Matrix;
    float qt_Opacity;
    // kfloat _pad0; float _pad1; float _pad2; // align to vec4
    vec4  pointA;   // (x_norm, L_mult, C_mult, unused)
    vec4  pointPA;
    vec4  pointCA;
    vec4  pointCB;
    vec4  pointPB;
    vec4  pointB;
} ubuf;

layout(binding = 1) uniform sampler2D source;

// ── Color space conversions ───────────────────────────────────────────────

vec3 srgbToLinear(vec3 c) {
    return mix(c / 12.92,
               pow((c + 0.055) / 1.055, vec3(2.4)),
               step(0.04045, c));
}

vec3 linearToSrgb(vec3 c) {
    c = clamp(c, 0.0, 1.0);
    return mix(12.92 * c,
               1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055,
               step(0.0031308, c));
}

vec3 linearToOklab(vec3 c) {
    // Column-major (GLSL): every column is a row of the mathematical matrix
    mat3 M1 = mat3(
        0.4122214708, 0.2119034982, 0.0883024619,
        0.5363325363, 0.6806995451, 0.2817188376,
        0.0514459929, 0.1073969566, 0.6299787005
    );
    vec3 lms  = M1 * c;
    vec3 lms_ = sign(lms) * pow(abs(lms), vec3(1.0 / 3.0));
    mat3 M2 = mat3(
        0.2104542553,  1.9779984951,  0.0259040371,
        0.7936177850, -2.4285922050,  0.7827717662,
       -0.0040720468,  0.4505937099, -0.8086757660
    );
    return M2 * lms_;
}

vec3 oklabToLinear(vec3 lab) {
    mat3 M2i = mat3(
        1.0000000000,  1.0000000000,  1.0000000000,
        0.3963377774, -0.1055613458, -0.0894841775,
        0.2158037573, -0.0638541728, -1.2914855480
    );
    vec3 lms_ = M2i * lab;
    vec3 lms  = lms_ * lms_ * lms_;
    mat3 M1i = mat3(
         4.0767416621, -1.2684380046, -0.0041960863,
        -3.3077115913,  2.6097574011, -0.7034186147,
         0.2309699292, -0.3413193965,  1.7076147010
    );
    return M1i * lms;
}

// ── Interpolation between checkpoints ─────────────────────────────────

vec2 evalPoints(float x) {
    // vec4.yz = (L_mult, C_mult)
    vec2 lc0 = ubuf.pointA.yz,  lc1 = ubuf.pointPA.yz;
    vec2 lc2 = ubuf.pointCA.yz, lc3 = ubuf.pointCB.yz;
    vec2 lc4 = ubuf.pointPB.yz, lc5 = ubuf.pointB.yz;

    float x0 = ubuf.pointA.x,  x1 = ubuf.pointPA.x;
    float x2 = ubuf.pointCA.x, x3 = ubuf.pointCB.x;
    float x4 = ubuf.pointPB.x, x5 = ubuf.pointB.x;

    vec2 result = lc0;

    float t;
    t = clamp((x - x0) / max(x1 - x0, 1e-5), 0.0, 1.0);
    result = mix(result, mix(lc0, lc1, t), step(x0, x));

    t = clamp((x - x1) / max(x2 - x1, 1e-5), 0.0, 1.0);
    result = mix(result, mix(lc1, lc2, t), step(x1, x));

    t = clamp((x - x2) / max(x3 - x2, 1e-5), 0.0, 1.0);
    result = mix(result, mix(lc2, lc3, t), step(x2, x));

    t = clamp((x - x3) / max(x4 - x3, 1e-5), 0.0, 1.0);
    result = mix(result, mix(lc3, lc4, t), step(x3, x));

    t = clamp((x - x4) / max(x5 - x4, 1e-5), 0.0, 1.0);
    result = mix(result, mix(lc4, lc5, t), step(x4, x));

    return result;
}

// ── Main ──────────────────────────────────────────────────────────────────

void main() {
    vec4 px = texture(source, qt_TexCoord0);
    if (px.a < 0.001) { fragColor = px; return; }

    // depremultiply
    vec3 rgb = px.rgb / px.a;

    // RGB → OKLCH
    vec3 lab = linearToOklab(srgbToLinear(rgb));
    float L = lab.x;
    float C = sqrt(lab.y * lab.y + lab.z * lab.z);
    float H = atan(lab.z, lab.y);

    // Add gradient multipliers
    vec2 lc = evalPoints(qt_TexCoord0.x);
    L *= lc.x;
    C *= lc.y;

    // OKLCH → RGB
    lab = vec3(L, C * cos(H), C * sin(H));
    vec3 outRgb = linearToSrgb(oklabToLinear(lab));

    // premultiply again
    fragColor = vec4(outRgb * px.a, px.a) * ubuf.qt_Opacity;
}