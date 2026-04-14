#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in float aShade;
layout(location = 3) in vec3 aTint;

uniform mat4 mvp;
uniform vec3 chunkOffset;

out vec2 TexCoord;
out float Shade;
out vec3 Tint;
out float FogDist;

void main() {
    vec3 worldPos = aPos + chunkOffset;
    gl_Position = mvp * vec4(worldPos, 1.0);
    TexCoord = aUV;
    Shade = aShade;
    Tint = aTint;
    // Java GL_FOG default (no NV extension): eye-plane distance = gl_Position.w
    // This is the depth along the camera forward axis, not spherical distance.
    FogDist = gl_Position.w;
}
