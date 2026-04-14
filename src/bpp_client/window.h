/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <string>

#include "glfw_context.h"

class Window {
public:
    Window(int width, int height, const std::string& title) {
        if (!glfwInit())
            throw std::runtime_error("Failed to init GLFW");

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (!handle) {
            glfwTerminate();
            throw std::runtime_error("Failed to create window");
        }

        glfwMakeContextCurrent(handle);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            glfwTerminate();
            throw std::runtime_error("Failed to init GLAD");
        }

        glViewport(0, 0, width, height);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        this->width = width;
        this->height = height;
        // Note: user pointer is set by Client, not here
    }

    // Called by Client after setting the user pointer
    void initCallbacks(GLFWwindow* win) {
        glfwSetFramebufferSizeCallback(win, framebufferSizeCallback);
    }

    ~Window() {
        if (handle) glfwDestroyWindow(handle);
        glfwTerminate();
    }

    // Non-copyable
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const { return glfwWindowShouldClose(handle); }
    void swapBuffers() { glfwSwapBuffers(handle); }
    void pollEvents() { glfwPollEvents(); }

    void setVsync(bool enabled) { glfwSwapInterval(enabled ? 1 : 0); }
    void setTitle(const std::string& title) { glfwSetWindowTitle(handle, title.c_str()); }
    void setCursorLocked(bool locked) {
        glfwSetInputMode(handle, GLFW_CURSOR,
            locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }

    int   getWidth()  const { return width; }
    int   getHeight() const { return height; }
    float getAspect() const { return (float)width / (float)height; }

    GLFWwindow* getHandle() const { return handle; }

private:
    static void framebufferSizeCallback(GLFWwindow* win, int w, int h) {
        auto* ctx = static_cast<GlfwContext*>(glfwGetWindowUserPointer(win));
        ctx->window->width = w;
        ctx->window->height = h;
        glViewport(0, 0, w, h);
    }

    GLFWwindow* handle = nullptr;
    int width = 0;
    int height = 0;
};