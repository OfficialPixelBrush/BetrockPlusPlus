// You can compile this with ./sokol-shdc --input ../shaders/basic.glsl --output ../src/bpp_client/basic_shader.h --slang glsl410
// Get sokol-shdc from https://github.com/floooh/sokol-tools-bin

#pragma sokol @vs vs
layout(binding = 0) uniform vs_params {
	mat4 mvp;
};

in vec3 pos;
in vec2 texcoord0;

out vec2 uv;

void main() {
	gl_Position = mvp * vec4(pos, 1.0);
	uv = texcoord0;
}
#pragma sokol @end

#pragma sokol @fs fs
layout(binding = 0) uniform texture2D tex;
layout(binding = 0) uniform sampler smp;

in vec2 uv;
out vec4 frag_color;

void main() {
	frag_color = texture(sampler2D(tex, smp), uv);
}
#pragma sokol @end

#pragma sokol @program basic vs fs