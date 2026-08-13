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

	// get the value at key as string, or _default if the key doesn't exist
	std::string GetAsString(const std::string& _key, std::string _default = "") {
		std::shared_lock lock(propertiesMutex);
		auto it = properties.find(_key);
		if (it == properties.end())
			return _default;
		return it->second;
	}

	// get the value at key as number, or _default if the key doesn't exist / doesn't parse
	template <std::integral num_type>
	num_type GetAsNumber(const std::string& _key, num_type _default = 0) {
		std::shared_lock lock(propertiesMutex);
		auto it = properties.find(_key);
		if (it == properties.end())
			return _default;

		try {
			return static_cast<num_type>(std::stoll(it->second));
		} catch (const std::exception& e) {
			std::cerr << "Error while parsing '" << _key << "' as number: " << e.what() << "\n";
			return _default;
		}
	}

	// get the value at key as boolean, or _default if the key doesn't exist / isn't a recognized boolean
	bool GetAsBoolean(const std::string& _key, bool _default = false) {
		std::shared_lock lock(propertiesMutex);
		auto it = properties.find(_key);
		if (it == properties.end())
			return _default;

		const std::string& val = it->second;
		if (val == "true" || val == "1")
			return true;
		if (val == "false" || val == "0")
			return false;

		// Unrecognized value: fall back to default
		return _default;
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