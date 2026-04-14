#version 330 core
in vec2 TexCoord;
in float Shade;
in vec3 Tint;
in float FogDist;

out vec4 FragColor;

uniform sampler2D tex;
uniform vec3 fogColor;
uniform float fogStart;
uniform float fogEnd;
uniform int fogMode;   // 0 = off, 1 = linear

void main() {
    vec4 texColor = texture(tex, TexCoord);
    if (texColor.a < 0.1) discard;

    vec3 color = texColor.rgb * Shade * Tint;

    if (fogMode == 0) {
        FragColor = vec4(color, texColor.a);
    } else {
        float fogFactor = clamp((fogEnd - FogDist) / (fogEnd - fogStart), 0.0, 1.0);
        FragColor = vec4(mix(fogColor, color, fogFactor), texColor.a);
    }
}