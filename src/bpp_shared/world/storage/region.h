/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "chunk.h"
#include "helpers/byteswap_compat.h"
#include "helpers/file_handle.h"
#include "numeric_structs.h"
#include <memory>
#include <mutex>

#define REGION_WIDTH 32
#define REGION_AREA REGION_WIDTH* REGION_WIDTH
#define SECTOR_SIZE 4096

inline std::string regionPositionToFileName(Int2 rpos) {
	return "r." + std::to_string(rpos.m_x) + "." + std::to_string(rpos.m_z) + ".mcr";
}

enum CompressorFormat {
	REGION_INVALID = 0,
	REGION_GZIP = 1,
	REGION_ZLIB = 2
};

struct FileHeaderEntry {
	uint32_t m_offset;
	uint8_t m_numberOfSectors;
	// TODO: Maybe store last-updated here?
};

struct ChunkHeaderEntry {
	uint32_t m_length;
	uint8_t m_format;
};

class Region {
public:
	Int32_2 m_rpos;
	std::mutex m_mutex;
	Region(Int32_2 rpos, std::string folderPath)
	    : m_rpos(rpos), regionFile(folderPath + "/" + regionPositionToFileName(rpos)) {
		// Cache our header
		readHeaderFromFile();
	}
	bool chunkExists(Int2 localcpos) {
		int index = localcpos.m_x + localcpos.m_z * 32;
		auto* rHeader = &regionHeader[index];
		return (rHeader->m_numberOfSectors != 0 && rHeader->m_offset != 0);
	}

	void AddChunk(std::shared_ptr<Chunk> chunk, int64_t timestamp, std::shared_ptr<const std::vector<Tag>> entities);
	std::shared_ptr<Chunk> GetChunk(Int32_2 cpos);

	// Read our header data into the "regionHeader"
	void readHeaderFromFile() {
		auto& file = regionFile.get();
		file.seekg(0); // Beginning of sector 0
		for (int i = 0; i < 1024; i++) {
			uint32_t entry;
			file.read(reinterpret_cast<char*>(&entry), 4);
			entry = __builtin_bswap32(entry);
			regionHeader[i].m_numberOfSectors = entry & 0xFF; // bottom 1 byte
			regionHeader[i].m_offset = entry >> 8;            // top 3 bytes
		}
	}

private:
	std::array<std::shared_ptr<Chunk>, REGION_AREA> chunks;
	std::vector<uint8_t> EncodeNbtData(const std::shared_ptr<Chunk>& chunk, int64_t timestamp,
	                                   std::shared_ptr<const std::vector<Tag>> entities);
	std::shared_ptr<Chunk> DecodeNbtData(const std::vector<uint8_t>& raw_data);
	std::string GetPath();
	std::array<FileHeaderEntry, 1024> regionHeader;
	FileHandle regionFile;
};