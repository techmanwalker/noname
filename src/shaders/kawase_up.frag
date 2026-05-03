#version 450

layout(location = 0) in  vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4      qt_Matrix;
    float     qt_Opacity;
    vec2      texelSize;
    float     offset;
} ubuf;

layout(binding = 1) uniform sampler2D source;

void main() {
    vec2 uv = qt_TexCoord0;
    vec2 o  = ubuf.offset * ubuf.texelSize;

    // 8-tap con pesos 2 en diagonales, 1 en cardinales → suma = 12
    vec4 color = vec4(0.0);
    color += texture(source, uv + vec2(-o.x * 2.0,  0.0      )      );
    color += texture(source, uv + vec2(-o.x,         o.y      )) * 2.0;
    color += texture(source, uv + vec2( 0.0,          o.y * 2.0)     );
    color += texture(source, uv + vec2( o.x,          o.y      )) * 2.0;
    color += texture(source, uv + vec2( o.x * 2.0,   0.0      )      );
    color += texture(source, uv + vec2( o.x,         -o.y      )) * 2.0;
    color += texture(source, uv + vec2( 0.0,         -o.y * 2.0)     );
    color += texture(source, uv + vec2(-o.x,         -o.y      )) * 2.0;

    fragColor = (color / 12.0) * ubuf.qt_Opacity;
}