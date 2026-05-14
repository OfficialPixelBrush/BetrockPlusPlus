/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <cstring>
#include "renderer.h"

#include "glfw_context.h"

struct InputEvent {
    int  code;
    int  action; // GLFW_PRESS, GLFW_RELEASE
    bool isMouse;
};

class Input {
public:
    #if defined(SDL_GPU)
    #elif defined(GLFW_OPENGL33)
    void init(GLFWwindow* window) {
        glfwSetKeyCallback(window, keyCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetCursorPosCallback(window, cursorCallback);
    }
    #endif

    // Called at the start of a tick — drains queue into stable state
    void drainEvents();

    bool isKeyHeld(int key)      const { return key >= 0 && key < 1024 && keys[key]; }
    bool isKeyPressed(int key)   const { return key >= 0 && key < 1024 && pressed[key]; }
    bool isMouseHeld(int btn)    const { return btn >= 0 && btn < 8 && mouse[btn]; }
    bool isMousePressed(int btn) const { return btn >= 0 && btn < 8 && mousePressed[btn]; }

    float mouseDeltaX() const { return deltaX; }
    float mouseDeltaY() const { return deltaY; }

    // Called at the end of a tick — clears one-shot flags
    void flush() {
        memset(pressed, 0, sizeof(pressed));
        memset(mousePressed, 0, sizeof(mousePressed));
        deltaX = deltaY = 0.0f;
    }

private:
    #if defined(SDL_GPU)
    #elif defined(GLFW_OPENGL33)
    static void keyCallback(GLFWwindow* window, int key, int, int action, int);
    static void mouseButtonCallback(GLFWwindow* window, int btn, int action, int);
    static void cursorCallback(GLFWwindow* window, double x, double y);
    #endif

    // Raw event queue — filled by callbacks, drained each tick
    std::vector<InputEvent> eventQueue;

    bool  keys[1024] = {};
    bool  pressed[1024] = {};
    bool  mouse[8] = {};
    bool  mousePressed[8] = {};

    float deltaX = 0.0f, deltaY = 0.0f;
    float lastX = 0.0f, lastY = 0.0f;
    bool  firstMouse = true;
};