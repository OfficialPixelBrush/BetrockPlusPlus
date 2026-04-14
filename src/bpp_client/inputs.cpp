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
    ctx->input->eventQueue.push_back({ key, action, false });
}

void Input::mouseButtonCallback(GLFWwindow* window, int btn, int action, int) {
    if (btn < 0 || btn >= 8) return;
    auto* ctx = static_cast<GlfwContext*>(glfwGetWindowUserPointer(window));
    ctx->input->eventQueue.push_back({ btn, action, true });
}

void Input::cursorCallback(GLFWwindow* window, double x, double y) {
    auto* ctx = static_cast<GlfwContext*>(glfwGetWindowUserPointer(window));
    Input* input = ctx->input;
    if (input->firstMouse) {
        input->lastX = (float)x;
        input->lastY = (float)y;
        input->firstMouse = false;
        return;
    }
    input->deltaX += (float)x - input->lastX;
    input->deltaY += input->lastY - (float)y; // flipped — screen Y is inverted
    input->lastX = (float)x;
    input->lastY = (float)y;
}

void Input::drainEvents() {
    std::vector<InputEvent> local;
    std::swap(local, eventQueue);

    for (auto& e : local) {
        if (e.isMouse) {
            if (e.action == GLFW_PRESS) {
                mouse[e.code] = true;
                mousePressed[e.code] = true;
            }
            else if (e.action == GLFW_RELEASE) {
                mouse[e.code] = false;
            }
        }
        else {
            if (e.action == GLFW_PRESS) {
                keys[e.code] = true;
                pressed[e.code] = true;
            }
            else if (e.action == GLFW_RELEASE) {
                keys[e.code] = false;
            }
        }
    }
}