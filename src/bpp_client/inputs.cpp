/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#include "inputs.h"
#include "glfw_context.h"

void Input::keyCallback(GLFWwindow* window, int key, int, int action, int) {
    if (key < 0 || key >= 1024) return;
    if (action == GLFW_REPEAT) return; // ignore repeats — held state handles continuous input
    auto* ctx = static_cast<GlfwContext*>(glfwGetWindowUserPointer(window));
    ctx->m_input->m_eventQueue.push_back({ key, action, false });
}

void Input::mouseButtonCallback(GLFWwindow* window, int btn, int action, int) {
    if (btn < 0 || btn >= 8) return;
    auto* ctx = static_cast<GlfwContext*>(glfwGetWindowUserPointer(window));
    ctx->m_input->m_eventQueue.push_back({ btn, action, true });
}

void Input::cursorCallback(GLFWwindow* window, double x, double y) {
    auto* ctx = static_cast<GlfwContext*>(glfwGetWindowUserPointer(window));
    Input* input = ctx->m_input;
    if (input->m_firstMouse) {
        input->m_lastX = float(x);
        input->m_lastY = float(y);
        input->m_firstMouse = false;
        return;
    }
    input->m_deltaX += float(x) - input->m_lastX;
    input->m_deltaY += input->m_lastY - float(y); // flipped — screen Y is inverted
    input->m_lastX = float(x);
    input->m_lastY = float(y);
}

void Input::drainEvents() {
    std::vector<InputEvent> local;
    std::swap(local, m_eventQueue);

    for (auto& e : local) {
        if (e.isMouse) {
            if (e.action == GLFW_PRESS) {
                m_mouse[e.code] = true;
                m_mousePressed[e.code] = true;
            }
            else if (e.action == GLFW_RELEASE) {
                m_mouse[e.code] = false;
            }
        }
        else {
            if (e.action == GLFW_PRESS) {
                m_keys[e.code] = true;
                m_pressed[e.code] = true;
            }
            else if (e.action == GLFW_RELEASE) {
                m_keys[e.code] = false;
            }
        }
    }
}