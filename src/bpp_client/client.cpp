/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "client.h"
#include <SDL3/SDL.h>
#include <atomic>

extern std::atomic<bool> shutdownRequested;

Client::Client() {}

void Client::Tick() {}

int Client::Run() {
	const uint64_t freq = SDL_GetPerformanceFrequency();
	uint64_t lastTime = SDL_GetPerformanceCounter();

	while (!shutdownRequested.load()) {
		uint64_t ticksRan = 0;
		uint64_t now = SDL_GetPerformanceCounter();
		float delta = static_cast<float>(now - lastTime) / static_cast<float>(freq);
		lastTime = now;
		accumulator += delta;

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT)
				shutdownRequested.store(true);
		}

		// Run ticks until caught up
		while (accumulator >= TICK_DELTA && ticksRan < MAX_TICKS_PER_FRAME) {
			Tick();
			accumulator -= TICK_DELTA;
			ticksRan++;
		}

		// Discard leftover time if we hit the cap
		if (ticksRan >= MAX_TICKS_PER_FRAME)
			accumulator = 0.0f;
	}
	return 0;
}