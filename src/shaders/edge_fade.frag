// simplified version of background_overlay.frag that only works on alpha
#version 450

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    
    vec2 outer_leftstop;
    vec2 inner_leftstop;
    vec2 inner_rightstop;
    vec2 outer_rightstop;
    bool switch_axis; // if true, apply through y axis instead of x
};

layout(binding = 1) uniform sampler2D sourceTexture;

void main() {
    // 1. Extract the base color of the input texture
    vec4 baseColor = texture(sourceTexture, qt_TexCoord0);
    
    // 2. Determine the eval axis according to switch_axis
    float t = switch_axis ? qt_TexCoord0.y : qt_TexCoord0.x;
    
    // 3. Calculate the alpha factor interpolating through the 4 stops
    float alphaFactor = 0.0;
    
    if (t < inner_leftstop.x) {
        // left/upper area: from outer_left to inner_left
        alphaFactor = smoothstep(outer_leftstop.x, inner_leftstop.x, t);
        alphaFactor = mix(outer_leftstop.y, inner_leftstop.y, alphaFactor);
    } else if (t > inner_rightstop.x) {
        // right/lower area: from inner_right to outer_right
        alphaFactor = smoothstep(outer_rightstop.x, inner_rightstop.x, t);
        alphaFactor = mix(outer_rightstop.y, inner_rightstop.y, alphaFactor);
    } else {
        // center area: between inner_left and inner_right
        float normCenter = (t - inner_leftstop.x) / max(0.001, (inner_rightstop.x - inner_leftstop.x));
        alphaFactor = mix(inner_leftstop.y, inner_rightstop.y, normCenter);
    }
    
    // 4. Multiply the alpha of the source texture by the calculated factor and the Qt opacity
    fragColor = baseColor * alphaFactor * qt_Opacity;
}