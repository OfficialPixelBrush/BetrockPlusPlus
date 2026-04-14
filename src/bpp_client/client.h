/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#pragma once
#include "glfw_context.h"
#include "inputs.h"
#include "window.h"
#include "world/World.h"

class Client {
public:
    Client();
    int run();
private:
    static constexpr float TICK_DELTA = 1.0f / 20.0f;
    static constexpr int   MAX_TICKS_PER_FRAME = 10;

    void tick();
    void render([[maybe_unused]] float partial_tick);

    Window      window;
    Input       input;
    GlfwContext ctx;
    WorldManager world;
    ClientPosition singlePlayerPos{};
    float       accumulator = 0.0f;
};