/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "renderer.h"
#include "misc.h"
#include "window.h"

Renderer::Renderer(Window& _window) : window(window.getHandle()) {
#ifndef NDEBUG
	const bool debug = true;
#else
	const bool debug = false;
#endif

	gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, debug, nullptr);

	if (!gpu)
		THROW_SDL_ERROR("Failed to create GPU device!");

	if (!SDL_ClaimWindowForGPUDevice(gpu, window))
		THROW_SDL_ERROR("Failed to claim window for GPU device!");
}

Renderer::~Renderer() {
	SDL_ReleaseWindowFromGPUDevice(gpu, window);
	SDL_DestroyGPUDevice(gpu);
}

void Renderer::render(int _partialTicks) {
	SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(gpu);
	if (!cmd)
		return;

	SDL_GPUTexture* swapchainTex;
	uint32_t width, height;

	if (SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &swapchainTex, &width, &height)) {
		SDL_GPUColorTargetInfo target{};
		target.texture = swapchainTex;
		target.clear_color = { 0.1f, 0.2f, 0.4f, 1.0f };
		target.load_op = SDL_GPU_LOADOP_CLEAR;
		target.store_op = SDL_GPU_STOREOP_STORE;

		SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &target, 1, nullptr);
		//TODO: Render stuff
		SDL_EndGPURenderPass(pass);
	}

	SDL_SubmitGPUCommandBuffer(cmd);
}