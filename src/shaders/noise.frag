#version 450

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 tSize;      // receptor component size
    vec2 nSize;      // noise texture size (ex. 64.0, 64.0)
    float intensity; // intensity control from caller
};

layout(binding = 1) uniform sampler2D sourceTexture; // previous effect texture
layout(binding = 2) uniform sampler2D noiseTexture;  // single channel noise pattern texture

// math function to break the repeating pattern
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

void main() {
    // 1. Extract the base color coming from the previous link on the effect chain
    vec4 baseColor = texture(sourceTexture, qt_TexCoord0);

    // 2. Calculate the noise UV coordinates to apply the tiling without repeating tiles effect
    vec2 uv = qt_TexCoord0 * (tSize / nSize);
    vec2 cell = floor(uv);
    vec2 localUV = fract(uv);
    
    // Pseudo-random transforms for each cell
    float angle = hash(cell) * 6.28318530718; 
    float flipX = hash(cell + vec2(0.1, 0.3)) > 0.5 ? -1.0 : 1.0;
    float flipY = hash(cell + vec2(0.7, 0.2)) > 0.5 ? -1.0 : 1.0;
    
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