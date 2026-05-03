#version 450

layout(location = 0) in  vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4      qt_Matrix;
    float     qt_Opacity;
    vec2      texelSize;  // 1/sourceWidth, 1/sourceHeight
    float     offset;
} ubuf;

layout(binding = 1) uniform sampler2D source;

void main() {
    vec2 uv = qt_TexCoord0;
    vec2 o  = ubuf.offset * ubuf.texelSize;

    vec4 color = texture(source, uv) * 4.0;
    color += texture(source, uv + vec2(-o.x,  o.y));
    color += texture(source, uv + vec2( o.x,  o.y));
    color += texture(source, uv + vec2(-o.x, -o.y));
    color += texture(source, uv + vec2( o.x, -o.y));

    fragColor = (color / 8.0) * ubuf.qt_Opacity;
}