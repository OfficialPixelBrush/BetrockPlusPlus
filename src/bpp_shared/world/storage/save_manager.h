/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "helpers/file_handle.h"
#include "java_math.h"
#include "nbt/nbt.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <libdeflate.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

struct levelData {
	int64_t m_RandomSeed = 0;
	Int3 m_spawnPoint{ 0, 0, 0 };
	int m_rainTime = 0;
	int m_thunderTime = 0;
	int8_t m_raining = 0;
	int64_t m_time = 0;
	int8_t m_thundering = 0;
	int m_version = 19132;
	int64_t m_lastPlayed = 0;
	std::string m_LevelName = "world";
	int64_t m_sizeOnDisk = 0;
};

struct SessionLock {
#ifdef _WIN32
	HANDLE handle = INVALID_HANDLE_VALUE;
#else
	int m_fd = -1;
#endif

	bool acquire(const std::string& path) {
#ifdef _WIN32
		handle = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
		                     FILE_ATTRIBUTE_NORMAL, nullptr);
		if (handle == INVALID_HANDLE_VALUE)
			return false;
#else
		m_fd = open(path.c_str(), O_RDWR | O_CREAT, 0644);
		if (m_fd < 0)
			return false;
		if (flock(m_fd, LOCK_EX | LOCK_NB) < 0) {
			close(m_fd);
			m_fd = -1;
			return false;
		}
#endif
		writeTimestamp();
		return true;
	}

	void release() {
#ifdef _WIN32
		if (handle != INVALID_HANDLE_VALUE) {
			CloseHandle(handle);
			handle = INVALID_HANDLE_VALUE;
		}
#else
		if (m_fd >= 0) {
			flock(m_fd, LOCK_UN);
			close(m_fd);
			m_fd = -1;
		}
#endif
	}

	~SessionLock() {
		release();
	}

private:
	void writeTimestamp() {
		int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
		                  std::chrono::system_clock::now().time_since_epoch())
		                  .count();
		uint8_t bytes[8];
		for (int i = 7; i >= 0; i--) {
			bytes[i] = now & 0xFF;
			now >>= 8;
		}
#ifdef _WIN32
		DWORD written;
		WriteFile(handle, bytes, 8, &written, nullptr);
#else
		write(m_fd, bytes, 8);
#endif
	}
};

struct SaveManager {
	bool initialize(const std::string& pSaveName, [[maybe_unused]] bool isMultiplayerSave = false) {
		SaveDirectory = pSaveName;

		// Make sure we have the necessary folders
		int necessaryFolders = 0;
		necessaryFolders += std::filesystem::create_directories(SaveDirectory + "/players");
		necessaryFolders += std::filesystem::create_directories(SaveDirectory + "/region");
		necessaryFolders += std::filesystem::create_directories(SaveDirectory + "/DIM-1/region");
		necessaryFolders += std::filesystem::create_directories(SaveDirectory + "/data");
		if (necessaryFolders)
			GlobalLogger().m_warn << "Failed to load " << necessaryFolders
			                    << " necessary folder(s) for level " + pSaveName + ".\n";

		if (!sessionLock.acquire(SaveDirectory + "/session.lock"))
			return false;
		if (!std::filesystem::exists(SaveDirectory + "/level.dat"))
			return false;
		worldFile = std::make_unique<FileHandle>(SaveDirectory + "/level.dat");
		if (!worldFile->get().is_open())
			return false;
		return true;
	}

	bool loadLevelData() {
		if (!worldFile || !worldFile->get().is_open())
			return false;

		std::fstream& stream = worldFile->get();

		// Read entire file into buffer
		stream.seekg(0, std::ios::end);
		size_t fileSize = stream.tellg();
		stream.seekg(0, std::ios::beg);
		std::vector<uint8_t> compressed(fileSize);
		stream.read(reinterpret_cast<char*>(compressed.data()), fileSize);

		// Decompress gzip
		libdeflate_decompressor* decompressor = libdeflate_alloc_decompressor();
		if (!decompressor)
			return false;
		std::vector<uint8_t> raw(1024 * 1024); // 1MB should be plenty for level.dat
		size_t actualSize = 0;
		libdeflate_result result = libdeflate_gzip_decompress(decompressor, compressed.data(), compressed.size(),
		                                                      raw.data(), raw.size(), &actualSize);
		libdeflate_free_decompressor(decompressor);
		if (result != LIBDEFLATE_SUCCESS)
			return false;
		raw.resize(actualSize);

		// Parse NBT
		NBTParser parser(raw.data(), raw.size());
		const Tag& data = parser.m_root.get("Data");

		currentLevelData.m_RandomSeed = data.get("RandomSeed").m_longValue;
		currentLevelData.m_spawnPoint.m_x = data.get("SpawnX").m_intValue;
		currentLevelData.m_spawnPoint.m_y = data.get("SpawnY").m_intValue;
		currentLevelData.m_spawnPoint.m_z = data.get("SpawnZ").m_intValue;
		currentLevelData.m_rainTime = data.get("rainTime").m_intValue;
		currentLevelData.m_thunderTime = data.get("thunderTime").m_intValue;
		currentLevelData.m_raining = data.get("raining").m_byteValue;
		currentLevelData.m_time = data.get("Time").m_longValue;
		currentLevelData.m_thundering = data.get("thundering").m_byteValue;
		currentLevelData.m_version = data.get("version").m_intValue;
		currentLevelData.m_lastPlayed = data.get("LastPlayed").m_longValue;
		currentLevelData.m_LevelName = data.get("LevelName").m_stringValue;
		currentLevelData.m_sizeOnDisk = data.get("SizeOnDisk").m_longValue;

		return true;
	}

	levelData& getLevelData() {
		return currentLevelData;
	}

	bool createNewWorld(const levelData& data) {
		std::error_code ec;
		std::filesystem::create_directories(SaveDirectory + "/players", ec);
		std::filesystem::create_directories(SaveDirectory + "/region", ec);
		std::filesystem::create_directories(SaveDirectory + "/DIM-1/region", ec);
		std::filesystem::create_directories(SaveDirectory + "/data", ec);
		return saveLevelFile(data);
	}

	bool saveLevelFile(const levelData& levelData) {
		// Back up existing level.dat if present
		if (std::filesystem::exists(SaveDirectory + "/level.dat")) {
			std::filesystem::copy_file(SaveDirectory + "/level.dat", SaveDirectory + "/level.dat_old",
			                           std::filesystem::copy_options::overwrite_existing);
		}

		Tag root;
		root.m_type = TAG_COMPOUND;
		root.m_name = "";

		Tag data;
		data.m_type = TAG_COMPOUND;
		data.m_name = "Data";

		Tag RandomSeed;
		RandomSeed.m_type = TAG_LONG;
		RandomSeed.m_name = "RandomSeed";
		RandomSeed.m_longValue = levelData.m_RandomSeed;
		Tag SpawnX;
		SpawnX.m_type = TAG_INT;
		SpawnX.m_name = "SpawnX";
		SpawnX.m_intValue = levelData.m_spawnPoint.m_x;
		Tag SpawnY;
		SpawnY.m_type = TAG_INT;
		SpawnY.m_name = "SpawnY";
		SpawnY.m_intValue = levelData.m_spawnPoint.m_y;
		Tag SpawnZ;
		SpawnZ.m_type = TAG_INT;
		SpawnZ.m_name = "SpawnZ";
		SpawnZ.m_intValue = levelData.m_spawnPoint.m_z;
		Tag rainTime;
		rainTime.m_type = TAG_INT;
		rainTime.m_name = "rainTime";
		rainTime.m_intValue = levelData.m_rainTime;
		Tag thunderTime;
		thunderTime.m_type = TAG_INT;
		thunderTime.m_name = "thunderTime";
		thunderTime.m_intValue = levelData.m_thunderTime;
		Tag raining;
		raining.m_type = TAG_BYTE;
		raining.m_name = "raining";
		raining.m_byteValue = levelData.m_raining;
		Tag Time;
		Time.m_type = TAG_LONG;
		Time.m_name = "Time";
		Time.m_longValue = levelData.m_time;
		Tag thundering;
		thundering.m_type = TAG_BYTE;
		thundering.m_name = "thundering";
		thundering.m_byteValue = levelData.m_thundering;
		Tag version;
		version.m_type = TAG_INT;
		version.m_name = "version";
		version.m_intValue = levelData.m_version;
		Tag LastPlayed;
		LastPlayed.m_type = TAG_LONG;
		LastPlayed.m_name = "LastPlayed";
		LastPlayed.m_longValue = levelData.m_lastPlayed;
		Tag LevelName;
		LevelName.m_type = TAG_STRING;
		LevelName.m_name = "LevelName";
		LevelName.m_stringValue = levelData.m_LevelName;
		Tag sizeOnDisk;
		sizeOnDisk.m_type = TAG_LONG;
		sizeOnDisk.m_name = "SizeOnDisk";
		sizeOnDisk.m_longValue = levelData.m_sizeOnDisk;

		data.m_compound["RandomSeed"] = RandomSeed;
		data.m_compound["SpawnX"] = SpawnX;
		data.m_compound["SpawnY"] = SpawnY;
		data.m_compound["SpawnZ"] = SpawnZ;
		data.m_compound["rainTime"] = rainTime;
		data.m_compound["thunderTime"] = thunderTime;
		data.m_compound["raining"] = raining;
		data.m_compound["Time"] = Time;
		data.m_compound["thundering"] = thundering;
		data.m_compound["version"] = version;
		data.m_compound["LastPlayed"] = LastPlayed;
		data.m_compound["LevelName"] = LevelName;
		data.m_compound["SizeOnDisk"] = sizeOnDisk;
		root.m_compound["Data"] = data;

		std::vector<uint8_t> raw;
		NBTwriter writer(raw, root);

		libdeflate_compressor* compressor = libdeflate_alloc_compressor(6);
		if (!compressor)
			return false;
		size_t maxSize = libdeflate_gzip_compress_bound(compressor, raw.size());
		std::vector<uint8_t> compressed(maxSize);
		size_t actualSize = libdeflate_gzip_compress(compressor, raw.data(), raw.size(), compressed.data(), maxSize);
		libdeflate_free_compressor(compressor);
		if (actualSize == 0)
			return false;
		compressed.resize(actualSize);

		std::ofstream file(SaveDirectory + "/level.dat", std::ios::binary);
		if (!file.is_open())
			return false;
		file.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
		if (!file.good())
			return false;

		// Reopen as FileHandle for future use
		worldFile = std::make_unique<FileHandle>(SaveDirectory + "/level.dat");
		return true;
	}

	static int64_t seedFromString(const std::string& input) {
		// If it's a plain number, use it directly
		try {
			size_t pos;
			int64_t numeric = std::stoll(input, &pos);
			if (pos == input.size())
				return numeric;
		} catch (...) {
		}

		// Otherwise hash it
		return hashCode(input);
	}

	Tag getPlayerNBT(const std::string& playerName) { // return by value
		std::string playerPath = SaveDirectory + "/players/" + playerName + ".dat";

		if (!std::filesystem::exists(playerPath)) {
			Tag fresh = getNewPlayerNBT();
			savePlayerNBT(playerName, fresh);
			return fresh;
		}

		// Load existing player file
		std::ifstream file(playerPath, std::ios::binary);
		if (!file.is_open())
			return getNewPlayerNBT();

		std::vector<uint8_t> compressed((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

		libdeflate_decompressor* decompressor = libdeflate_alloc_decompressor();
		if (!decompressor)
			return getNewPlayerNBT();

		std::vector<uint8_t> raw(1024 * 1024);
		size_t actualSize = 0;
		libdeflate_result result = libdeflate_gzip_decompress(decompressor, compressed.data(), compressed.size(),
		                                                      raw.data(), raw.size(), &actualSize);
		libdeflate_free_decompressor(decompressor);
		if (result != LIBDEFLATE_SUCCESS)
			return getNewPlayerNBT();

		raw.resize(actualSize);
		NBTParser parser(raw.data(), raw.size());
		return parser.m_root;
	}

	Tag getNewPlayerNBT() {
		Tag root;
		root.m_type = TAG_COMPOUND;
		root.m_name = "";

		Tag Motion;
		Motion.m_type = TAG_LIST;
		Motion.m_name = "Motion";
		Motion.m_listType = TAG_DOUBLE;
		Tag SleepTimer;
		SleepTimer.m_type = TAG_SHORT;
		SleepTimer.m_name = "SleepTimer";
		SleepTimer.m_shortValue = 0;
		Tag Health;
		Health.m_type = TAG_SHORT;
		Health.m_name = "Health";
		Health.m_shortValue = 20;
		Tag Air;
		Air.m_type = TAG_SHORT;
		Air.m_name = "Air";
		Air.m_shortValue = 300;
		Tag OnGround;
		OnGround.m_type = TAG_BYTE;
		OnGround.m_name = "OnGround";
		OnGround.m_byteValue = 0;
		Tag Dimension;
		Dimension.m_type = TAG_INT;
		Dimension.m_name = "Dimension";
		Dimension.m_intValue = 0;
		Tag Rotation;
		Rotation.m_type = TAG_LIST;
		Rotation.m_name = "Rotation";
		Rotation.m_listType = TAG_FLOAT;
		Tag FallDistance;
		FallDistance.m_type = TAG_FLOAT;
		FallDistance.m_name = "FallDistance";
		FallDistance.m_floatValue = 0.0f;
		Tag Sleeping;
		Sleeping.m_type = TAG_BYTE;
		Sleeping.m_name = "Sleeping";
		Sleeping.m_byteValue = 0;
		Tag Pos;
		Pos.m_type = TAG_LIST;
		Pos.m_name = "Pos";
		Pos.m_listType = TAG_DOUBLE;
		Tag DeathTime;
		DeathTime.m_type = TAG_SHORT;
		DeathTime.m_name = "DeathTime";
		DeathTime.m_shortValue = 0;
		Tag Fire;
		Fire.m_type = TAG_SHORT;
		Fire.m_name = "Fire";
		Fire.m_shortValue = -20;
		Tag HurtTime;
		HurtTime.m_type = TAG_SHORT;
		HurtTime.m_name = "HurtTime";
		HurtTime.m_shortValue = 0;
		Tag AttackTime;
		AttackTime.m_type = TAG_SHORT;
		AttackTime.m_name = "AttackTime";
		AttackTime.m_shortValue = 0;
		Tag Inventory;
		Inventory.m_type = TAG_LIST;
		Inventory.m_name = "Inventory";
		Inventory.m_listType = TAG_COMPOUND;

		// Initialize our position with a default
		Tag posX;
		posX.m_type = TAG_DOUBLE;
		posX.m_doubleValue = -1;
		Tag posY;
		posY.m_type = TAG_DOUBLE;
		posY.m_doubleValue = -1000000;
		Tag posZ;
		posZ.m_type = TAG_DOUBLE;
		posZ.m_doubleValue = -1;
		Pos.m_list.push_back(posX);
		Pos.m_list.push_back(posY);
		Pos.m_list.push_back(posZ);

		Tag rotX;
		rotX.m_type = TAG_FLOAT;
		rotX.m_floatValue = 0.0f;
		Tag rotY;
		rotY.m_type = TAG_FLOAT;
		rotY.m_floatValue = 0.0f;
		Rotation.m_list.push_back(rotX);
		Rotation.m_list.push_back(rotY);

		// Initialize our velocity with a default
		Tag movX;
		movX.m_type = TAG_DOUBLE;
		movX.m_doubleValue = 0.0;
		Tag movY;
		movY.m_type = TAG_DOUBLE;
		movY.m_doubleValue = 0.0;
		Tag movZ;
		movZ.m_type = TAG_DOUBLE;
		movZ.m_doubleValue = 0.0;
		Motion.m_list.push_back(movX);
		Motion.m_list.push_back(movY);
		Motion.m_list.push_back(movZ);

		root.m_compound["Motion"] = Motion;
		root.m_compound["SleepTimer"] = SleepTimer;
		root.m_compound["Health"] = Health;
		root.m_compound["Air"] = Air;
		root.m_compound["OnGround"] = OnGround;
		root.m_compound["Dimension"] = Dimension;
		root.m_compound["Rotation"] = Rotation;
		root.m_compound["FallDistance"] = FallDistance;
		root.m_compound["Sleeping"] = Sleeping;
		root.m_compound["Pos"] = Pos;
		root.m_compound["DeathTime"] = DeathTime;
		root.m_compound["Fire"] = Fire;
		root.m_compound["HurtTime"] = HurtTime;
		root.m_compound["AttackTime"] = AttackTime;
		root.m_compound["Inventory"] = Inventory;
		return root;
	}

	bool savePlayerNBT(const std::string& playerName, Tag& playerData) const {
		std::vector<uint8_t> raw;
		NBTwriter writer(raw, playerData);

		libdeflate_compressor* compressor = libdeflate_alloc_compressor(6);
		if (!compressor)
			return false;
		size_t maxSize = libdeflate_gzip_compress_bound(compressor, raw.size());
		std::vector<uint8_t> compressed(maxSize);
		size_t actualSize = libdeflate_gzip_compress(compressor, raw.data(), raw.size(), compressed.data(), maxSize);
		libdeflate_free_compressor(compressor);
		if (actualSize == 0)
			return false;
		compressed.resize(actualSize);

		std::string finalPath = SaveDirectory + "/players/" + playerName + ".dat";
		std::string tmpPath = finalPath + ".tmp";

		std::ofstream file(tmpPath, std::ios::binary);
		if (!file.is_open())
			return false;
		file.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
		file.flush();
		if (!file.good())
			return false;
		file.close();

		std::filesystem::rename(tmpPath, finalPath);
		return true;
	}

	void release() {
		sessionLock.release();
	}

	~SaveManager() {
		release();
	}

private:
	std::string SaveDirectory;
	std::unique_ptr<FileHandle> worldFile;
	SessionLock sessionLock;
	levelData currentLevelData;
};