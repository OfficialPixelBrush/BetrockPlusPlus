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

#include "glfw_context.h"

struct InputEvent {
    int  code;
    int  action; // GLFW_PRESS, GLFW_RELEASE
    bool isMouse;
};

class Input {
public:
    void init(GLFWwindow* window) {
        glfwSetKeyCallback(window, keyCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetCursorPosCallback(window, cursorCallback);
    }

    // Called at the start of a tick — drains queue into stable state
    void drainEvents();

    bool isKeyHeld(int key)      const { return key >= 0 && key < 1024 && m_keys[key]; }
    bool isKeyPressed(int key)   const { return key >= 0 && key < 1024 && m_pressed[key]; }
    bool isMouseHeld(int btn)    const { return btn >= 0 && btn < 8 && m_mouse[btn]; }
    bool isMousePressed(int btn) const { return btn >= 0 && btn < 8 && m_mousePressed[btn]; }

    float mouseDeltaX() const { return m_deltaX; }
    float mouseDeltaY() const { return m_deltaY; }

    // Called at the end of a tick — clears one-shot flags
    void flush() {
        memset(m_pressed, 0, sizeof(m_pressed));
        memset(m_mousePressed, 0, sizeof(m_mousePressed));
        m_deltaX = m_deltaY = 0.0f;
    }

private:
    static void keyCallback(GLFWwindow* window, int key, int, int action, int);
    static void mouseButtonCallback(GLFWwindow* window, int btn, int action, int);
    static void cursorCallback(GLFWwindow* window, double x, double y);

    // Raw event queue — filled by callbacks, drained each tick
    std::vector<InputEvent> m_eventQueue;

    bool  m_keys[1024] = {};
    bool  m_pressed[1024] = {};
    bool  m_mouse[8] = {};
    bool  m_mousePressed[8] = {};

    float m_deltaX = 0.0f, m_deltaY = 0.0f;
    float m_lastX = 0.0f, m_lastY = 0.0f;
    bool  m_firstMouse = true;
};