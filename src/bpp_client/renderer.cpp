/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "renderer.h"
#include "basic_shader.h"
#include "blocks.h"
#include "chunk.h"
#include "logger.h"
#include "renderers/blocks/mapping.h"
#include "window.h"
#include "world/generator/overworld/chunk_gen.h"
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_video.h>
#include <cstdint>
#include <cstdlib>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class CubeFace {
	Front,
	Back,
	Right,
	Left,
	Top,
	Bottom,
};

void AddFace(std::vector<Vertex>& _vertices, std::vector<uint16_t>& _indices, const Int3& _p, BlockType _block,
             CubeFace _face) {
	std::array<Int3, 4> corners;
	Vec3 normal;

	switch (_face) {
	case CubeFace::Front:
		corners = { {
			_p + Int3{ 0, 0, 1 },
			_p + Int3{ 1, 0, 1 },
			_p + Int3{ 1, 1, 1 },
			_p + Int3{ 0, 1, 1 },
		} };
		normal = { 0.0f, 0.0f, 1.0f };
		break;

	case CubeFace::Back:
		corners = { {
			_p + Int3{ 1, 0, 0 },
			_p + Int3{ 0, 0, 0 },
			_p + Int3{ 0, 1, 0 },
			_p + Int3{ 1, 1, 0 },
		} };
		normal = { 0.0f, 0.0f, -1.0f };
		break;

	case CubeFace::Right:
		corners = { {
			_p + Int3{ 1, 0, 1 },
			_p + Int3{ 1, 0, 0 },
			_p + Int3{ 1, 1, 0 },
			_p + Int3{ 1, 1, 1 },
		} };
		normal = { 1.0f, 0.0f, 0.0f };
		break;

	case CubeFace::Left:
		corners = { {
			_p + Int3{ 0, 0, 0 },
			_p + Int3{ 0, 0, 1 },
			_p + Int3{ 0, 1, 1 },
			_p + Int3{ 0, 1, 0 },
		} };
		normal = { -1.0f, 0.0f, 0.0f };
		break;

	case CubeFace::Top:
		corners = { {
			_p + Int3{ 0, 1, 1 },
			_p + Int3{ 1, 1, 1 },
			_p + Int3{ 1, 1, 0 },
			_p + Int3{ 0, 1, 0 },
		} };
		normal = { 0.0f, 1.0f, 0.0f };
		break;

	case CubeFace::Bottom:
		corners = { {
			_p + Int3{ 0, 0, 0 },
			_p + Int3{ 1, 0, 0 },
			_p + Int3{ 1, 0, 1 },
			_p + Int3{ 0, 0, 1 },
		} };
		normal = { 0.0f, -1.0f, 0.0f };
		break;
	}

	int gridIndex = BLOCK_TEXTURE_INDEX[_block];
	Int2 gridCoord = { gridIndex % 16, gridIndex / 16 };

	constexpr int ATLAS_SIZE = 256;
	constexpr int TILE_PIXELS = 16;

	constexpr float TILE_SIZE = static_cast<float>(TILE_PIXELS) / ATLAS_SIZE;

	Float2 uvMin = { gridCoord.x * TILE_SIZE, gridCoord.y * TILE_SIZE };

	std::array<Float2, 4> uvs = { {
		{ uvMin.x, uvMin.y + TILE_SIZE },
		{ uvMin.x + TILE_SIZE, uvMin.y + TILE_SIZE },
		{ uvMin.x + TILE_SIZE, uvMin.y },
		{ uvMin.x, uvMin.y },
	} };

	uint16_t base = static_cast<uint16_t>(_vertices.size());

	for (int i = 0; i < 4; ++i) {
		const auto& c = corners[i];

		_vertices.emplace_back(Vertex{
		    .position = { float(c.x), float(c.y), float(c.z) },
		    .uv = uvs[i],
		});
	}

	_indices.push_back(base);
	_indices.push_back(base + 2);
	_indices.push_back(base + 1);

	_indices.push_back(base);
	_indices.push_back(base + 3);
	_indices.push_back(base + 2);
}

BlockType GetBlockSafe(Chunk& _chunk, Int3 _pos) {
	if (_pos.x < 0 || _pos.x >= CHUNK_WIDTH || _pos.y < 0 || _pos.y >= CHUNK_HEIGHT || _pos.z < 0 ||
	    _pos.z >= CHUNK_WIDTH) {
		return BLOCK_AIR;
	}

	return _chunk.GetBlock(_pos);
}

bool IsBlockTransparent(BlockType _block) {
	return BLOCK_TEXTURE_INDEX[_block] == 0; // This obviously isn't correct, only temporary
}

void BuildCube(Chunk& _chunk, std::vector<Vertex>& _vertices, std::vector<uint16_t>& _indices, const Int3& _worldPos,
               const Int3& _chunkPos, BlockType _block) {
	if (IsBlockTransparent(GetBlockSafe(_chunk, _worldPos + Int3{ 0, 0, 1 })))
		AddFace(_vertices, _indices, _chunkPos, _block, CubeFace::Front);

	if (IsBlockTransparent(GetBlockSafe(_chunk, _worldPos + Int3{ 0, 0, -1 })))
		AddFace(_vertices, _indices, _chunkPos, _block, CubeFace::Back);

	if (IsBlockTransparent(GetBlockSafe(_chunk, _worldPos + Int3{ 1, 0, 0 })))
		AddFace(_vertices, _indices, _chunkPos, _block, CubeFace::Right);

	if (IsBlockTransparent(GetBlockSafe(_chunk, _worldPos + Int3{ -1, 0, 0 })))
		AddFace(_vertices, _indices, _chunkPos, _block, CubeFace::Left);

	if (IsBlockTransparent(GetBlockSafe(_chunk, _worldPos + Int3{ 0, 1, 0 })))
		AddFace(_vertices, _indices, _chunkPos, _block, CubeFace::Top);

	if (IsBlockTransparent(GetBlockSafe(_chunk, _worldPos + Int3{ 0, -1, 0 })))
		AddFace(_vertices, _indices, _chunkPos, _block, CubeFace::Bottom);
}

[[nodiscard]] MeshData GenerateChunkMesh(Chunk& _chunk) {
	MeshData data;

	for (int y = 0; y < CHUNK_HEIGHT; ++y) {
		for (int z = 0; z < CHUNK_WIDTH; ++z) {
			for (int x = 0; x < CHUNK_WIDTH; ++x) {
				Int3 pos{ x, y, z };
				BlockType block = GetBlockSafe(_chunk, pos);
				if (IsBlockTransparent(block)) // This is temporary too
					continue;

				BuildCube(_chunk, data.vertices, data.indices, pos, pos, block);
			}
		}
	}

	return data;
}

struct VsParams {
	glm::mat4 mvp;
};
VsParams CalculateVsParams(Camera& _camera, float _aspect) {
	glm::mat4 viewProj = _camera.GetViewProj(_aspect);

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(0.08f, 0.08f, 0.08f));
	model = glm::translate(model, glm::vec3(-8.0f, -32.0f, 0.0f));

	return { .mvp = viewProj * model };
}

Renderer::Renderer(Window& _window) : window(_window) {
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Required for macOS
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	glContext = SDL_GL_CreateContext(window.GetHandle());

	// We don't need anything extra for OpenGL
	sg_setup(sg_desc{ .logger = sg_logger{ .func = SokolLogCallback } });

	// Load some assets
	auto surface = SDL_LoadSurface("terrain.png");
	if (!surface) {
		GlobalLogger().error << "Please add terrain.png to the current working directory!\n";
		abort();
	}

	sg_image_desc imgDesc{};
	imgDesc.data.mip_levels[0] = { .ptr = surface->pixels, .size = static_cast<size_t>((surface->w * surface->h * 4)) };
	imgDesc.width = surface->w;
	imgDesc.height = surface->h;

	terrainTex = sg_make_image(imgDesc);
	SDL_DestroySurface(surface);

	sampler = sg_make_sampler(sg_sampler_desc{});
	terrainTexView = sg_make_view(sg_view_desc{ .texture = { .image = terrainTex } });

	// Setup rendering
	Chunk* chunk = new Chunk();
	OverworldGenerator generator(123);
	generator.GenerateChunk(*chunk);

	MeshData meshData = GenerateChunkMesh(*chunk);
	delete chunk;

	mesh.vertexBuffer = sg_make_buffer(
	    sg_buffer_desc{ .data = { meshData.vertices.data(), meshData.vertices.size() * sizeof(Vertex) } });

	mesh.indexBufffer = sg_make_buffer(
	    sg_buffer_desc{ .usage = { .index_buffer = true },
	                    .data = { meshData.indices.data(), meshData.indices.size() * sizeof(uint16_t) } });
	mesh.indexCount = meshData.indices.size();

	printf("vertices=%zu indices=%zu\n", meshData.vertices.size(), meshData.indices.size());

	sg_pipeline_desc desc{};
	desc.layout.buffers[0].stride = sizeof(Vertex);

	desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
	desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;

	desc.shader = sg_make_shader(basic_shader_desc(sg_query_backend()));
	desc.index_type = SG_INDEXTYPE_UINT16;
	desc.cull_mode = SG_CULLMODE_BACK;
	desc.depth.write_enabled = true;
	desc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;

	pipeline = sg_make_pipeline(&desc);
}

Renderer::~Renderer() {
	sg_shutdown();
	SDL_GL_DestroyContext(glContext);
}

sg_swapchain Renderer::GetSwapchain() {
	Int2 framebufferSize = window.GetFramebufferSize();
	return { .invalid = false,
		     .width = framebufferSize.x,
		     .height = framebufferSize.y,
		     .sample_count = 1,
		     .color_format = SG_PIXELFORMAT_RGBA8,
		     .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
		     .gl = { .framebuffer = 0 } }; // 0 is our window
}

void Renderer::Render(Camera& _camera, int _partialTicks) {
	float fbAspect = window.GetFramebufferAspect();
	VsParams vsParams = CalculateVsParams(_camera, fbAspect);

	sg_pass_action passAction{};
	passAction.colors[0].load_action = SG_LOADACTION_CLEAR;
	passAction.colors[0].clear_value = { 1, 0, 0, 1 };

	sg_begin_pass(sg_pass{ .action = passAction, .swapchain = GetSwapchain() });

	sg_apply_pipeline(pipeline);

	sg_bindings bindings{};
	bindings.vertex_buffers[0] = mesh.vertexBuffer;
	bindings.index_buffer = mesh.indexBufffer;
	bindings.samplers[0] = sampler;
	bindings.views[0] = terrainTexView;
	sg_apply_bindings(bindings);

	sg_apply_uniforms(0, SG_RANGE(vsParams));

	sg_draw(0, mesh.indexCount, 1);

	sg_end_pass();

	// Frame end
	sg_commit();
	window.SwapBuffers();
}

void Renderer::SokolLogCallback(const char* _tag, uint32_t _logLevel, uint32_t _logItemId, const char* _messageOrNull,
                                uint32_t _lineNr, const char* _filenameOrNull, void* _userData) {
	if (!_messageOrNull)
		return;

	LogLevel level;
	switch (_logLevel) {
	case 0: // panic
	case 1: // error
		level = LOG_ERROR;
		break;
	case 2: // warning
		level = LOG_WARNING;
		break;
	case 3: // info
		level = LOG_INFO;
		break;
	default:
		level = LOG_DEBUG;
		break;
	}

	GlobalLogger().Log(_messageOrNull, level);
}