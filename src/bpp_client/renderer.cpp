/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "renderer.h"
#include "logger.h"
#include "misc.h"
#include "window.h"

Renderer::Renderer(Window& _window) : window(_window.GetHandle()) {
#ifndef NDEBUG
	const bool debug = true;
#else
	const bool debug = false;
#endif

	int numDrivers = SDL_GetNumGPUDrivers();
	GlobalLogger().info << "GPU drivers found: " << numDrivers << "\n";
	for (int i = 0; i < numDrivers; i++) {
		GlobalLogger().info << "  [" << i << "] " << SDL_GetGPUDriver(i) << "\n";
	}

	SDL_PropertiesID props = SDL_CreateProperties();
	SDL_SetStringProperty(props, SDL_PROP_GPU_DEVICE_CREATE_NAME_STRING, "vulkan");
	SDL_SetNumberProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);
	SDL_SetNumberProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, debug);

	gpu = SDL_CreateGPUDeviceWithProperties(props);
	SDL_DestroyProperties(props);
	if (SDL_GetError()[0] != '\0') {
		GlobalLogger().error << "Critical error during initialization, aborting.\n";
		GlobalLogger().error << SDL_GetError() << "\n";
		throw std::runtime_error("Couldn't initialize SDL!");
		return;
	}

	SDL_ClaimWindowForGPUDevice(gpu, window);
}

Renderer::~Renderer() {
	SDL_ReleaseWindowFromGPUDevice(gpu, window);
	SDL_DestroyGPUDevice(gpu);
}

void Renderer::Render(int _partialTicks) {
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