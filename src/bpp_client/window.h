/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#pragma once
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_video.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <string>

#include "glfw_context.h"
#include "logger.h"

class Window {
public:
    Window(int width, int height, const std::string& title) {
        #if defined(SDL_GPU)
        // Inits Video backend
        if (!SDL_Init(SDL_INIT_VIDEO))
            GlobalLogger().error << "(SDL) Failed to initialize SDL!\n";
        SDL_Window *window{SDL_CreateWindow(
            title.c_str(),
            width,
            height,
            SDL_WINDOW_VULKAN
        )};
        if (!window)
            GlobalLogger().error << "(SDL) Failed to create window!\n";

        SDL_GPUDevice *device{SDL_CreateGPUDevice(
            // SPIRV for Vulkan, MSL for Metal, DXIL for DirectX 12
            SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_DXIL,
            true,
            nullptr
        )};
        if (!device)
            GlobalLogger().error << "(SDL) Failed to create device!\n";

        SDL_ClaimWindowForGPUDevice(device, window);
        #elif defined(GLFW_OPENGL33)
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

        if (!gladLoadGLLoader(GLADloadproc(glfwGetProcAddress))) {
            glfwTerminate();
            throw std::runtime_error("Failed to init GLAD");
        }

        glViewport(0, 0, width, height);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        #endif

        this->width = width;
        this->height = height;
        GlobalLogger().info << "Window initialized!\n";
        // Note: user pointer is set by Client, not here
    }

    // Called by Client after setting the user pointer
    void initCallbacks(GLFWwindow* win) {
        #if defined(SDL_GPU)
        #elif defined(GLFW_OPENGL33)
        glfwSetFramebufferSizeCallback(win, framebufferSizeCallback);
        #endif
    }

    ~Window() {
        #if defined(SDL_GPU)
        #elif defined(GLFW_OPENGL33)
        if (handle) glfwDestroyWindow(handle);
        glfwTerminate();
        #endif
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
    float getAspect() const { return float(width) / float(height); }

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