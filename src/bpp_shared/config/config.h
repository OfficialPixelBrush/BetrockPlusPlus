/*
 * Copyright (c) 2025, MINA <github.com/9mina>
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#pragma once
#include "logger.h"

#include <iostream>
#include <shared_mutex>

#include <string>
#include <unordered_map>

class Config {
private:
	struct TransparentHasher {
		using IsTransparent = void;
		size_t operator()(std::string_view _sv) const {
			std::hash<std::string_view> hasher;
			return hasher(_sv);
		}
	};

	using ConfType = std::unordered_map<std::string, std::string, TransparentHasher, std::equal_to<>>;

public:
	// get the value at key or a the default mapped_type if key doesn't exist
	std::string_view Get(const std::string& _key) noexcept;

	std::string GetAsString(const std::string& _key) {
		return std::string(this->Get(_key));
	}

	// get the value at key as number
	template <std::integral num_type>
	num_type GetAsNumber(const std::string& _key) {
		return std::stoll(std::string(this->Get(_key)));
	}

	// get the value at key as boolean
	bool GetAsBoolean(const std::string& _key) {
		std::string val = std::string(this->Get(_key));
		try {
			if (val == "true" || val == "1")
				return true;
			// All other cases default to false
			/*
			if (val == "false" || val == "0")
				return false;
			*/
		} catch (const std::exception& e) {
			std::cerr << "Error while writing: " << e.what() << "\n";
		}
		return false;
	}

	// set value at key.
	// will create key if it doesn't exist.
	void Set(const std::string& _key, std::string_view _value) noexcept;

	// overwrite the properties in memory
	void Overwrite(const ConfType& _config) noexcept;

	// read a properties file from disk into memory.
	// returns false on error.
	bool LoadFromDisk() noexcept;

	// save the properties in memory to disk.
	// returns false on error.
	bool SaveToDisk() const noexcept;

	// set a new path to the properties file
	void SetPath(std::string_view _path) noexcept;

	// get the current properties path
	std::string_view GetPath() const noexcept;

	Config(const std::string& _path);
	~Config() = default;

private:
	Config(const Config&) = delete;
	Config(const Config&&) = delete;

	Config& operator=(const Config&) = delete;
	Config& operator=(const Config&&) = delete;

	std::shared_mutex propertiesMutex;

	std::string path;
	ConfType properties;
};