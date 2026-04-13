#pragma once

// Forward declarations
class Window;
class Input;

// Shared context passed as the GLFW user pointer so both
// Window and Input callbacks can coexist on one pointer
struct GlfwContext {
    Window* window = nullptr;
    Input* input = nullptr;
};