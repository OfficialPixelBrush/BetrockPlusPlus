/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "client.h"

// This window size seems really random but its the size beta uses
Client::Client() : window(854, 480, "Betrock++") {
    // Set up shared context before registering any callbacks
    ctx = { &window, &input };
    glfwSetWindowUserPointer(window.getHandle(), &ctx);

    input.init(window.getHandle());
    window.initCallbacks(window.getHandle());

    window.setCursorLocked(true);
    window.setVsync(true);
    GlobalLogger().info << "Client initialized\n";
}

void Client::tick() {
    this->world.tick(std::vector<ClientPosition>{this->singlePlayerPos});
}

void Client::render([[maybe_unused]] float partial_tick) {

}

int Client::run() {
    float lastTime = float(glfwGetTime());

    while (!window.shouldClose()) {
        int   ticks_ran = 0;
        float now = float(glfwGetTime());
        float delta = now - lastTime;
        lastTime = now;
        accumulator += delta;

        window.pollEvents();

        // Run ticks until caught up, but cap to avoid spiraling on slow frames
        while (accumulator >= TICK_DELTA && ticks_ran < MAX_TICKS_PER_FRAME) {
            input.drainEvents();
            tick();
            input.flush();
            accumulator -= TICK_DELTA;
            ticks_ran++;
        }

        // Discard leftover time if we hit the cap
        if (ticks_ran == MAX_TICKS_PER_FRAME)
            accumulator = 0.0f;

        render(accumulator / TICK_DELTA);
        window.swapBuffers();
    }
    return 0;
}