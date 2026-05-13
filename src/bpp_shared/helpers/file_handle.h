/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
#include <fstream>
#include <memory>
#include <string>

class FileHandle {
public:
    explicit FileHandle(const std::string& path)
        : stream(path, std::ios::in | std::ios::out | std::ios::binary)
    {
        if (!stream.is_open()) {
            throw std::runtime_error("Failed to open file: " + path);
        }
    }

    std::fstream& get() { return stream; }

private:
    std::fstream stream;
};