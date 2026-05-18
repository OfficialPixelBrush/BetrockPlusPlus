/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once

// Platform
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_NAME "Windows"
#elif defined(__APPLE__)
    #define PLATFORM_NAME "macOS"
#elif defined(__ANDROID__)
    #define PLATFORM_NAME "Android"
#elif defined(__linux__)
    #define PLATFORM_NAME "Linux"
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    #define PLATFORM_NAME "BSD"
#elif defined(__unix__) || defined(__unix)
    #define PLATFORM_NAME "Unix"
#elif defined(__SWITCH__)
    #define PLATFORM_NAME "Nintendo Switch"
#elif defined(__3DS__)
    #define PLATFORM_NAME "Nintendo 3DS"
#else
    #define PLATFORM_NAME "Unknown Platform"
#endif


// CPU Architecture
#if defined(__x86_64__) || defined(_M_X64)
    #define ARCH_NAME "x86_64"
#elif defined(__i386__) || defined(_M_IX86)
    #define ARCH_NAME "x86"
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define ARCH_NAME "ARM64"
#elif defined(__arm__) || defined(_M_ARM)
    #define ARCH_NAME "ARM"
#elif defined(__riscv)
    #define ARCH_NAME "RISC-V"
#elif defined(__powerpc64__) || defined(__ppc64__)
    #define ARCH_NAME "PowerPC64"
#elif defined(__powerpc__) || defined(__ppc__)
    #define ARCH_NAME "PowerPC"
#else
    #define ARCH_NAME "Unknown Arch"
#endif

// Build type
#ifdef NDEBUG
#define BUILD_MODE "Release"
#else
#define BUILD_MODE "Debug"
#endif