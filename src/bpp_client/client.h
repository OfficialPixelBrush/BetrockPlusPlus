/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "client_pos.h"
#include "input.h"
#include "renderer.h"
#include "window.h"

class Client {
public:
	Client();
	int run();

private:
	static constexpr float TICK_DELTA = 1.0f / 20.0f;
	static constexpr int MAX_TICKS_PER_FRAME = 10;

	void tick();
	void render([[maybe_unused]] float partial_tick);

	Window m_window;
	Input m_input;
	Renderer m_renderer;
	//TODO: Should this be here?
	ClientPosition m_singlePlayerPos{};
	float m_accumulator = 0.0f;
};