/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "client_pos.h"
#include "logger.h"
#include "runtime.h"

class Client {
public:
	Client();
	int Run();
	bool LoadIntoWorld(std::string _levelPath);

	Runtime gameRuntime;

private:
	static constexpr float TICK_DELTA = 1.0f / 20.0f;
	static constexpr int MAX_TICKS_PER_FRAME = 10;

	void Tick();
	float accumulator = 0.0f;
};