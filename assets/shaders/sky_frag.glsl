#version 330 core
in vec2 TexCoord;
in vec4 VColor;

out vec4 FragColor;

uniform vec3      color;
uniform float     alpha;
uniform sampler2D tex;
uniform int       useTex;         // 1 = sample texture
uniform int       useVertexColor; // 1 = use per-vertex colour (glow fan)

void main() {
    if (useTex == 1) {
        vec4 t = texture(tex, TexCoord);
        if (t.a < 0.05) discard;
        FragColor = vec4(t.rgb * color, t.a * alpha);
    } else if (useVertexColor == 1) {
        FragColor = VColor;
    } else {
        FragColor = vec4(color, alpha);
    }
}
