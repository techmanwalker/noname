#version 450

layout(location = 0) in  vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4  qt_Matrix;
    float qt_Opacity;
    // kfloat _pad0; float _pad1; float _pad2; // align to vec4
    vec3  pointA;   // (x_norm, L_mult, C_mult)
    vec3  pointPA;
    vec3  pointCA;
    vec3  pointCB;
    vec3  pointPB;
    vec3  pointB;
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

// Cubic Hermite spline with Catmull-Rom-style finite-difference tangents:
// the same curve as curveValue() in Background.qml (keep them in sync).
// C1-continuous, so there is no slope jump at the control points.

// One segment p1 -> p2; p0 and p3 only supply the tangents. .x is the
// position, .yz the (L_mult, C_mult) pair, interpolated together.
vec2 hermiteSegment(float x, vec3 p0, vec3 p1, vec3 p2, vec3 p3) {
    float h = p2.x - p1.x;
    if (h <= 1e-6) return p1.yz; // zero-length segment

    float t = (x - p1.x) / h;

    // If the neighbours coincide in x, fall back to the segment's own width,
    // like curveValue does.
    float d1 = p2.x - p0.x;
    float d2 = p3.x - p1.x;
    vec2 m1 = (p2.yz - p0.yz) / (d1 != 0.0 ? d1 : h);
    vec2 m2 = (p3.yz - p1.yz) / (d2 != 0.0 ? d2 : h);

    float t2 = t * t;
    float t3 = t2 * t;

    return ( 2.0 * t3 - 3.0 * t2 + 1.0) * p1.yz
         + (       t3 - 2.0 * t2 + t  ) * h * m1
         + (-2.0 * t3 + 3.0 * t2      ) * p2.yz
         + (       t3 -       t2      ) * h * m2;
}

vec2 evalPoints(float x) {
    vec3 a  = ubuf.pointA,  pa = ubuf.pointPA, ca = ubuf.pointCA;
    vec3 cb = ubuf.pointCB, pb = ubuf.pointPB, b  = ubuf.pointB;

    float cx = clamp(x, a.x, b.x);

    // Same segment pick as curveValue(): the first segment whose right end
    // is >= cx. The end points are repeated at the borders, as curveValue
    // does, which makes the outermost tangents one-sided.
    vec2 lc;
    if      (cx <= pa.x) lc = hermiteSegment(cx, a,  a,  pa, ca);
    else if (cx <= ca.x) lc = hermiteSegment(cx, a,  pa, ca, cb);
    else if (cx <= cb.x) lc = hermiteSegment(cx, pa, ca, cb, pb);
    else if (cx <= pb.x) lc = hermiteSegment(cx, ca, cb, pb, b);
    else                 lc = hermiteSegment(cx, cb, pb, b,  b);

    // The spline can undershoot; a negative multiplier would invert L or flip
    // the hue (negative chroma), so floor it.
    return max(lc, 0.0);
}

bool inGamut(vec3 c) {
    return all(greaterThanEqual(c, vec3(-1e-4))) &&
           all(lessThanEqual(c, vec3(1.0 + 1e-4)));
}

// Reduces chroma (L and H fixed) until the color fits in linear sRGB
vec3 gamutMapLinear(float L, float C, float H) {
    vec2 hue = vec2(cos(H), sin(H));
    vec3 lin = oklabToLinear(vec3(L, C * hue));
    if (inGamut(lin)) return lin;

    float lo = 0.0, hi = C;
    for (int i = 0; i < 8; i++) {
        float mid = 0.5 * (lo + hi);
        if (inGamut(oklabToLinear(vec3(L, mid * hue)))) lo = mid;
        else hi = mid;
    }
    return oklabToLinear(vec3(L, lo * hue));
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
    vec3 outRgb = linearToSrgb(gamutMapLinear(L, C, H));

    // premultiply again
    fragColor = vec4(outRgb * px.a, px.a) * ubuf.qt_Opacity;
}