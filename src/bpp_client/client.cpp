/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "client.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>

// This m_window size seems really random but its the size beta uses
Client::Client() : m_window(854, 480, "Betrock++") {
	// Set up shared context before registering any callbacks
	m_ctx = { &m_window, &m_input };
	glfwSetWindowUserPointer(m_window.getHandle(), &m_ctx);

	m_input.init(m_window.getHandle());
	m_window.initCallbacks(m_window.getHandle());

	m_window.setCursorLocked(true);
	m_window.setVsync(true);

	GlobalLogger().info << "Client initialized\n";
}

void Client::tick() {}

void Client::render([[maybe_unused]] float partial_tick) {}

int Client::run() {
	float lastTime = float(glfwGetTime());

	while (!m_window.shouldClose()) {
		int ticks_ran = 0;
		float now = float(glfwGetTime());
		float delta = now - lastTime;
		lastTime = now;
		m_accumulator += delta;

		m_window.pollEvents();

		// Run ticks until caught up, but cap to avoid spiraling on slow frames
		while (m_accumulator >= TICK_DELTA && ticks_ran < MAX_TICKS_PER_FRAME) {
			m_input.drainEvents();
			tick();
			m_input.flush();
			m_accumulator -= TICK_DELTA;
			ticks_ran++;
		}

		// Discard leftover time if we hit the cap
		if (ticks_ran == MAX_TICKS_PER_FRAME)
			m_accumulator = 0.0f;

		render(m_accumulator / TICK_DELTA);
		m_window.swapBuffers();
	}
	return 0;
}