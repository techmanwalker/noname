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
    return fract(sin(dot(p + vec2(seedTime), vec2(127.1, 311.7))) * 43758.5453123);
}

void main() {
    float noise = hash(qt_TexCoord0);
    
    // monochromatic noise, only red channel persists in memory
    fragColor = vec4(noise, 0.0, 0.0, 1.0) * qt_Opacity;
}