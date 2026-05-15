/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
#include "helpers/file_handle.h"
#include "NBT/NBT.h"
#include "java_math.h"
#include <chrono>
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <libdeflate.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#endif

struct levelData {
    int64_t RandomSeed = 0;
    Int3 spawnPoint{ 0, 0, 0 };
    int rainTime = 0;
    int thunderTime = 0;
    int8_t raining = 0;
    int64_t time = 0;
    int8_t thundering = 0;
    int version = 19132;
    int64_t lastPlayed = 0;
    std::string LevelName = "world";
    int64_t sizeOnDisk = 0;
};

struct SessionLock {
    #ifdef _WIN32
    HANDLE handle = INVALID_HANDLE_VALUE;
    #else
    int fd = -1;
    #endif
    
    bool acquire(const std::string& path) {
        #ifdef _WIN32
        handle = CreateFileA(
            path.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0, nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );
        if (handle == INVALID_HANDLE_VALUE)
            return false;
        #elif defined(__SWITCH__)
        // Switch (libnx) has no flock() — just open/create the file
        fd = open(path.c_str(), O_RDWR | O_CREAT, 0644);
        if (fd < 0)
            return false;
        // No advisory lock available; single-process environment so this is fine
        #else
        fd = open(path.c_str(), O_RDWR | O_CREAT, 0644);
        if (fd < 0)
            return false;
        if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
            close(fd);
            fd = -1;
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
        if (fd >= 0) {
            #if !defined(__SWITCH__)
            flock(fd, LOCK_UN);
            #endif
            close(fd);
            fd = -1;
        }
        #endif
    }

    ~SessionLock() { release(); }

private:
    void writeTimestamp() {
        int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        uint8_t bytes[8];
        for (int i = 7; i >= 0; i--) {
            bytes[i] = now & 0xFF;
            now >>= 8;
        }
        #ifdef _WIN32
        DWORD written;
        WriteFile(handle, bytes, 8, &written, nullptr);
        #else
        write(fd, bytes, 8);
        #endif
    }
};

struct SaveManager {
    bool initialize(const std::string& pSaveName, bool isMultiplayerSave = false) {
        SaveDirectory = pSaveName;
        if (!sessionLock.acquire(SaveDirectory + "/session.lock"))
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
        if (!decompressor) return false;
        std::vector<uint8_t> raw(1024 * 1024); // 1MB should be plenty for level.dat
        size_t actualSize = 0;
        libdeflate_result result = libdeflate_gzip_decompress(
            decompressor,
            compressed.data(), compressed.size(),
            raw.data(), raw.size(),
            &actualSize
        );
        libdeflate_free_decompressor(decompressor);
        if (result != LIBDEFLATE_SUCCESS) return false;
        raw.resize(actualSize);

        // Parse NBT
        NBTParser parser(raw.data(), raw.size());
        const Tag& data = parser.root.get("Data");

        currentLevelData.RandomSeed =   data.get("RandomSeed").longValue;
        currentLevelData.spawnPoint.x = data.get("SpawnX").intValue;
        currentLevelData.spawnPoint.y = data.get("SpawnY").intValue;
        currentLevelData.spawnPoint.z = data.get("SpawnZ").intValue;
        currentLevelData.rainTime =     data.get("rainTime").intValue;
        currentLevelData.thunderTime =  data.get("thunderTime").intValue;
        currentLevelData.raining =      data.get("raining").byteValue;
        currentLevelData.time =         data.get("Time").longValue;
        currentLevelData.thundering =   data.get("thundering").byteValue;
        currentLevelData.version =      data.get("version").intValue;
        currentLevelData.lastPlayed =   data.get("LastPlayed").longValue;
        currentLevelData.LevelName =    data.get("LevelName").stringValue;
        currentLevelData.sizeOnDisk =   data.get("sizeOnDisk").longValue;

        return true;
    }

	levelData& getLevelData() { return currentLevelData; }

    bool createNewWorld(const levelData& data) {
        // Set SaveDirectory before acquiring lock
        if (!std::filesystem::create_directory(SaveDirectory))
            return false;
        if (!sessionLock.acquire(SaveDirectory + "/session.lock"))
            return false;
        std::filesystem::create_directories(SaveDirectory + "/region");
        std::filesystem::create_directories(SaveDirectory + "/DIM-1/region");
        std::filesystem::create_directories(SaveDirectory + "/players");
        std::filesystem::create_directories(SaveDirectory + "/data");
        return saveLevelFile(data);
    }

	bool saveLevelFile(const levelData& levelData) {
        // Back up existing level.dat if present
        if (std::filesystem::exists(SaveDirectory + "/level.dat")) {
            std::filesystem::copy_file(
                SaveDirectory + "/level.dat",
                SaveDirectory + "/level_old.dat",
                std::filesystem::copy_options::overwrite_existing
            );
        }

        Tag root;
        root.type = TAG_COMPOUND;
        root.name = "";

        Tag data;
        data.type = TAG_COMPOUND;
        data.name = "Data";

        Tag RandomSeed;  RandomSeed.type = TAG_LONG;   RandomSeed.name = "RandomSeed";  RandomSeed.longValue = levelData.RandomSeed;
        Tag SpawnX;      SpawnX.type = TAG_INT;    SpawnX.name = "SpawnX";      SpawnX.intValue = levelData.spawnPoint.x;
        Tag SpawnY;      SpawnY.type = TAG_INT;    SpawnY.name = "SpawnY";      SpawnY.intValue = levelData.spawnPoint.y;
        Tag SpawnZ;      SpawnZ.type = TAG_INT;    SpawnZ.name = "SpawnZ";      SpawnZ.intValue = levelData.spawnPoint.z;
        Tag rainTime;    rainTime.type = TAG_INT;    rainTime.name = "rainTime";    rainTime.intValue = levelData.rainTime;
        Tag thunderTime; thunderTime.type = TAG_INT;    thunderTime.name = "thunderTime"; thunderTime.intValue = levelData.thunderTime;
        Tag raining;     raining.type = TAG_BYTE;   raining.name = "raining";     raining.byteValue = levelData.raining;
        Tag Time;        Time.type = TAG_LONG;   Time.name = "Time";        Time.longValue = levelData.time;
        Tag thundering;  thundering.type = TAG_BYTE;   thundering.name = "thundering";  thundering.byteValue = levelData.thundering;
        Tag version;     version.type = TAG_INT;    version.name = "version";     version.intValue = levelData.version;
        Tag LastPlayed;  LastPlayed.type = TAG_LONG;   LastPlayed.name = "LastPlayed";  LastPlayed.longValue = levelData.lastPlayed;
        Tag LevelName;   LevelName.type = TAG_STRING; LevelName.name = "LevelName";   LevelName.stringValue = levelData.LevelName;
        Tag sizeOnDisk;  sizeOnDisk.type = TAG_LONG;   sizeOnDisk.name = "sizeOnDisk";  sizeOnDisk.longValue = levelData.sizeOnDisk;

        data.compound["RandomSeed"] = RandomSeed;
        data.compound["SpawnX"] = SpawnX;
        data.compound["SpawnY"] = SpawnY;
        data.compound["SpawnZ"] = SpawnZ;
        data.compound["rainTime"] = rainTime;
        data.compound["thunderTime"] = thunderTime;
        data.compound["raining"] = raining;
        data.compound["Time"] = Time;
        data.compound["thundering"] = thundering;
        data.compound["version"] = version;
        data.compound["LastPlayed"] = LastPlayed;
        data.compound["LevelName"] = LevelName;
        data.compound["sizeOnDisk"] = sizeOnDisk;
        root.compound["Data"] = data;

        std::vector<uint8_t> raw;
        NBTwriter writer(raw, root);

        libdeflate_compressor* compressor = libdeflate_alloc_compressor(6);
        if (!compressor) return false;
        size_t maxSize = libdeflate_gzip_compress_bound(compressor, raw.size());
        std::vector<uint8_t> compressed(maxSize);
        size_t actualSize = libdeflate_gzip_compress(compressor, raw.data(), raw.size(), compressed.data(), maxSize);
        libdeflate_free_compressor(compressor);
        if (actualSize == 0) return false;
        compressed.resize(actualSize);

        std::ofstream file(SaveDirectory + "/level.dat", std::ios::binary);
        if (!file.is_open()) return false;
        file.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
        if (!file.good()) return false;

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
        }
        catch (...) {}

        // Otherwise hash it
		return hashCode(input);
    }

    void release() {
        sessionLock.release();
    }

    ~SaveManager() { release(); }

private:
    std::string SaveDirectory;
    std::unique_ptr<FileHandle> worldFile;
    SessionLock sessionLock;
	levelData currentLevelData;
};