/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include <atomic>
extern std::atomic<bool> shutdownRequested;

#include <SDL3/SDL_events.h>

#include "client.h"
#include "logger.h"

// This m_window size seems really random but its the size beta uses
Client::Client() : m_window({ 854, 480 }, "Betrock++", { WindowMode::WINDOWED_RESIZABLE }), m_renderer(m_window) {
	m_window.setCursorCapture(true);

	GlobalLogger().info << "Client initialized\n";
}

void Client::tick() {}

int Client::run() {
	const uint64_t freq = SDL_GetPerformanceFrequency();
	uint64_t lastTime = SDL_GetPerformanceCounter();

	while (!shutdownRequested.load()) {
		uint64_t ticksRan = 0;
		uint64_t now = SDL_GetPerformanceCounter();
		float delta = static_cast<float>(now - lastTime) / static_cast<float>(freq);
		lastTime = now;
		m_accumulator += delta;

		m_input.newFrame();

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT)
				shutdownRequested.store(true);

			m_input.handleEvent(event);
		}

		// Run ticks until caught up, but cap to avoid spiraling on slow frames
		while (m_accumulator >= TICK_DELTA && ticksRan < MAX_TICKS_PER_FRAME) {
			tick();
			m_accumulator -= TICK_DELTA;
			ticksRan++;
		}

		// Discard leftover time if we hit the cap
		if (ticksRan == MAX_TICKS_PER_FRAME)
			m_accumulator = 0.0f;

		m_renderer.render(m_accumulator / TICK_DELTA);
	}
	return 0;
}