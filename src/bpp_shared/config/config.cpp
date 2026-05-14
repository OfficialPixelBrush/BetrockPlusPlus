/*
 * Copyright (c) 2025, MINA <github.com/9mina>
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#include "config.h"

#include <fstream>
#include <iostream>
#include <mutex>

// TODO: Replace std::string_view with std::filesystem::path
Config::Config(const std::string& pPath) {
	this->path = pPath;
}

std::string_view Config::Get(const std::string &key) noexcept {
	std::shared_lock read_lock{this->properties_mutex};
	return this->properties.contains(key) ? this->properties.at(key) : std::string_view();
}

void Config::Set(const std::string &key, std::string_view value) noexcept {
	std::unique_lock write_lock{this->properties_mutex};
	this->properties[key] = value;
}

// overwrite the properties in memory
void Config::Overwrite(const ConfType &config) noexcept {
	std::unique_lock write_lock{this->properties_mutex};
	this->properties = config;
}

bool Config::LoadFromDisk() noexcept {
	std::ifstream file(this->path);

	if (!file.is_open()) {
		GlobalLogger().warn << "**** Error opening properties file (load). Attempting to create new file...\n";
		return false;
	}

	std::unique_lock lock{this->properties_mutex};

	this->properties.clear();

	std::string line;
	while (std::getline(file, line)) {
		// Skip empty lines and comments
		if (line.empty() || line[0] == '#')
			continue;

		auto delimiterPos = line.find('=');
		if (delimiterPos == std::string::npos) {
			GlobalLogger().error << "**** Invalid line in properties file: " << line << "\n";
			continue;
		}

		std::string key = line.substr(0, delimiterPos);
		std::string value = line.substr(delimiterPos + 1);

		// Trim whitespace (optional)
		key.erase(key.find_last_not_of(" \t\n\r\f\v") + 1);
		value.erase(0, value.find_first_not_of(" \t\n\r\f\v"));

		properties[key] = value;
	}
	file.close();
	return true;
}

bool Config::SaveToDisk() const noexcept {
	std::ofstream file(this->path);
	if (!file.is_open()) {
		GlobalLogger().error << "**** Error opening properties file (save). \n";
		return false;
	}

	try {
		for (const auto &[key, value] : this->properties) {
			file << key << "=" << value << "\n";
		}
	} catch (const std::exception &e) {
		GlobalLogger().error << "**** Error while writing properties file: " << e.what() << "\n";
		return false;
	}

	GlobalLogger().info << "Properties file saved successfully.\n";
	file.close();
	return true;
}

void Config::SetPath(std::string_view pPath) noexcept { this->path = pPath; }

std::string_view Config::GetPath() const noexcept { return this->path; }