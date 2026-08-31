#version 450

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 nSize;      // noise texture size (ex. 64.0, 64.0)
    float intensity; // intensity control from caller
    float cellSeed;  // per-instance offset so tile transforms reroll each run
};

layout(binding = 1) uniform sampler2D sourceTexture; // previous effect texture
layout(binding = 2) uniform sampler2D noiseTexture;  // single channel noise pattern texture

// Integer bit-mixing hash (pcg3d — Jarzynski & Olano, JCGT 2020). The
// previous fract()-based hash repeats exactly every ~9.7-10.3 input units —
// invisible for per-pixel hashing, but glaring for per-tile hashing, since
// only ~15-25 cells fit across a screen. Bit mixing has no such periodicity.
uvec3 pcg3d(uvec3 v) {
    v = v * 1664525u + 1013904223u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v ^= v >> 16u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    return v;
}

vec3 hash32(vec2 p, float seed) {
    uvec3 h = pcg3d(uvec3(uvec2(p), uint(seed)));
    return vec3(h) * (1.0 / 4294967296.0); // normalize to [0,1)
}

void main() {
    // 1. Extract the base color coming from the previous link on the effect chain
    vec4 baseColor = texture(sourceTexture, qt_TexCoord0);

    // 2. Calculate the noise UV coordinates to apply the tiling without repeating tiles effect.
    // Driven by gl_FragCoord (the real physical pixel grid) rather than
    // qt_TexCoord0 * tSize (logical size): at a fractional Wayland scale,
    // logical width/height don't line up 1:1 with the pixels the fragment
    // shader actually runs on, so tile boundaries sat at fractional physical
    // positions and jittered every resize frame. gl_FragCoord has no such
    // ambiguity — it's exactly the physical pixel being shaded, always.
    vec2 uv = gl_FragCoord.xy / nSize;
    vec2 cell = floor(uv);
    vec2 localUV = fract(uv);
    
    // Pseudo-random transforms for each cell
    vec3 rnd = hash32(cell, cellSeed);
    float angle  = rnd.x * 6.28318530718;
    float flipX  = rnd.y > 0.5 ? -1.0 : 1.0;
    float flipY  = rnd.z > 0.5 ? -1.0 : 1.0;
    
    localUV = (localUV - 0.5) * vec2(flipX, flipY) + 0.5;
    
    float s = sin(angle);
    float c = cos(angle);
    mat2 rotationMatrix = mat2(c, -s, s, c);
    vec2 rotatedUV = rotationMatrix * (localUV - 0.5) + 0.5;
    
    // 3. Take the grain sample
    float noise = texture(noiseTexture, rotatedUV).r;
    
    // 4. Blend the noise directly over the RGB channels of the base color

    vec3 finalColor = baseColor.rgb + (noise - 0.5) * intensity;
    fragColor = vec4(finalColor, baseColor.a) * qt_Opacity;
}