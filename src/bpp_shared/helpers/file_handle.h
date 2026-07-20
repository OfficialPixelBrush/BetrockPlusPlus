/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include <fstream>
#include <memory>
#include <string>

class FileHandle {
public:
	FileHandle() = default;
	FileHandle(const std::string& _path) : stream(_path, std::ios::in | std::ios::out | std::ios::binary) {
		if (!stream.is_open()) {
			throw std::runtime_error("Failed to open file: " + _path);
		}
	}

	std::fstream& Get() {
		return stream;
	}

private:
	std::fstream stream;
};