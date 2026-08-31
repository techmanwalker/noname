#version 450

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float seedTime; // generated on startup
};

// Pseudo-random noise generator
float hash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

void main() {
    float noise = hash(gl_FragCoord.xy + seedTime);
    
    // monochromatic noise, only red channel persists in memory
    fragColor = vec4(noise, 0.0, 0.0, 1.0) * qt_Opacity;
}