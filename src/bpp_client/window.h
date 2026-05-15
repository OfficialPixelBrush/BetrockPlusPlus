/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#pragma once
#include "renderer.h"
#if defined(SDL_GPU)
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_video.h>
#elif defined(GLFW_OPENGL33)
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#endif
#include <stdexcept>
#include <string>

#include "glfw_context.h"
#include "logger.h"
#include "renderer.h"

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

        if (!SDL_ClaimWindowForGPUDevice(device, window))
            GlobalLogger().error << "(SDL) Failed to claim window for GPU device!\n";

        SDL_ShowWindow(window);

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
    #if defined(SDL_GPU)
    #elif defined(GLFW_OPENGL33)
    void initCallbacks(GLFWwindow* win) {
        glfwSetFramebufferSizeCallback(win, framebufferSizeCallback);
    }
    #endif

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

    bool shouldClose() const {
        #if defined(SDL_GPU)
        return false;
        #elif defined(GLFW_OPENGL33)
        return glfwWindowShouldClose(handle);
        #endif
    }
    void swapBuffers() {
        #if defined(SDL_GPU)
        #elif defined(GLFW_OPENGL33)
        glfwSwapBuffers(handle);
        #endif
    }
    void pollEvents() {
        #if defined(SDL_GPU)
        #elif defined(GLFW_OPENGL33)
        glfwPollEvents();
        #endif
    }

    void setVsync(bool enabled) {
        #if defined(SDL_GPU)
        #elif defined(GLFW_OPENGL33)
        glfwSwapInterval(enabled ? 1 : 0);
        #endif
    }
    void setTitle(const std::string& title) {
        #if defined(SDL_GPU)
        #elif defined(GLFW_OPENGL33)
        glfwSetWindowTitle(handle, title.c_str());
        #endif
    }
    void setCursorLocked(bool locked) {
        #if defined(SDL_GPU)
        #elif defined(GLFW_OPENGL33)
        glfwSetInputMode(handle, GLFW_CURSOR,
            locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        #endif
    }

    int   getWidth()  const { return width; }
    int   getHeight() const { return height; }
    float getAspect() const { return float(width) / float(height); }

    #if defined(SDL_GPU)
    #elif defined(GLFW_OPENGL33)
    GLFWwindow* getHandle() const { return handle; }
    #endif

private:
    #if defined(SDL_GPU)
    #elif defined(GLFW_OPENGL33)
    static void framebufferSizeCallback(GLFWwindow* win, int w, int h) {
        auto* ctx = static_cast<GlfwContext*>(glfwGetWindowUserPointer(win));
        ctx->window->width = w;
        ctx->window->height = h;
        glViewport(0, 0, w, h);
    }
    GLFWwindow* handle = nullptr;
    #endif

    int width = 0;
    int height = 0;
};