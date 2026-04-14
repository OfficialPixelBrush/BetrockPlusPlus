#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;

uniform mat4  mvp;
uniform int   useVertexColor;
uniform float verticalOffset;

out vec2 TexCoord;
out vec4 VColor;

void main() {
    gl_Position = mvp * vec4(aPos + vec3(0.0, verticalOffset, 0.0), 1.0);
    gl_PointSize = 1.5;
    TexCoord     = aUV;
    VColor       = aColor;
}
