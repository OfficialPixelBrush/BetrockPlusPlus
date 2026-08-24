/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#include "list_parser.h"
#include "logger.h"

std::vector<std::string> ListParser::Read(Target _target) {
	std::vector<std::string> values;
	std::ifstream file(TARGET_PATHS[size_t(_target)]);
	if (!file) {
		GlobalLogger().warn << TARGET_PATHS[size_t(_target)] << " doesn't exist! Creating it..." << "\n";
		std::ofstream createFile(TARGET_PATHS[size_t(_target)]);
		createFile.close();
		return {};
	}
	for (std::string username; getline(file, username);) {
		values.push_back(username);
	}
	file.close();
	return values;
}

bool ListParser::Write(std::vector<std::string>& _list, Target _target) {
	std::ofstream file(TARGET_PATHS[size_t(_target)]);
	for (auto entry : _list) {
		file << entry << "\n";
	}
	file.close();
	return true;
}