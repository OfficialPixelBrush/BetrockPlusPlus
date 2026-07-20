/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "region.h"
#include "chunk.h"
#include "logger.h"
#include "nbt/nbt.h"
#include "tile_entities/tile_entity.h"
#include <cstdint>
#include <fstream>
#include <libdeflate.h>
#include <memory>
#include <string>

void Region::AddChunk(std::shared_ptr<Chunk> _chunk, int64_t _timestamp,
                      std::shared_ptr<const std::vector<Tag>> _entities) {
	std::lock_guard<std::mutex> lock(mutex);
	auto compressed = EncodeNbtData(_chunk, _timestamp, _entities);
	if (compressed.empty())
		return;

	// Chunk header: 4 bytes length + 1 byte compression type
	uint32_t payloadLength = uint32_t(compressed.size()) + 1; // +1 for the format byte
	uint32_t sectorsNeeded = (payloadLength + 4 + SECTOR_SIZE - 1) / SECTOR_SIZE;

	// Find a free run of sectors in the file
	// Build an occupancy set from the header
	std::vector<bool> occupied;
	for (int i = 0; i < 1024; i++) {
		if (regionHeader[i].offset == 0)
			continue;
		uint32_t end = regionHeader[i].offset + regionHeader[i].numberOfSectors;
		if (end > occupied.size())
			occupied.resize(end, false);
		for (uint32_t s = regionHeader[i].offset; s < end; s++)
			occupied[s] = true;
	}

	// Find first free run of `sectorsNeeded` sectors starting at 2 (after header)
	uint32_t chosenOffset = 0;
	for (uint32_t s = 2;; s++) {
		bool fits = true;
		for (uint32_t j = 0; j < sectorsNeeded; j++) {
			if (s + j < occupied.size() && occupied[s + j]) {
				fits = false;
				break;
			}
		}
		if (fits) {
			chosenOffset = s;
			break;
		}
	}

	// Write chunk data at the chosen sector
	auto& file = regionFile.Get();
	file.seekp(chosenOffset * SECTOR_SIZE);

	// 4-byte big-endian length
	uint32_t lengthBE = __builtin_bswap32(payloadLength);
	file.write(reinterpret_cast<char*>(&lengthBE), 4);

	// 1-byte compression type
	uint8_t format = REGION_ZLIB;
	file.write(reinterpret_cast<char*>(&format), 1);

	// Compressed data
	file.write(reinterpret_cast<char*>(compressed.data()), compressed.size());

	// Pad to sector boundary
	size_t written = 5 + compressed.size();
	size_t padded = sectorsNeeded * SECTOR_SIZE;
	if (written < padded) {
		std::vector<char> pad(padded - written, 0);
		file.write(pad.data(), pad.size());
	}

	// Update header entry
	Int2 local{ _chunk->cpos.x & 31, _chunk->cpos.z & 31 };
	int index = local.x + local.z * 32;
	regionHeader[index].offset = chosenOffset;
	regionHeader[index].numberOfSectors = uint8_t(sectorsNeeded);

	// Write updated header entry to file
	file.seekp(index * 4);
	uint32_t entry = (chosenOffset << 8) | uint8_t(sectorsNeeded);
	entry = __builtin_bswap32(entry);
	file.write(reinterpret_cast<char*>(&entry), 4);
	file.flush();
}

std::shared_ptr<Chunk> Region::GetChunk(Int32_2 _cpos) {
	std::lock_guard<std::mutex> lock(mutex);
	Int2 local{ _cpos.x & 31, _cpos.z & 31 };
	int index = local.x + local.z * 32;

	if (!ChunkExists(local))
		return nullptr;

	uint32_t offset = regionHeader[index].offset;
	if (offset == 0)
		return nullptr;

	auto& file = regionFile.Get();
	file.seekg(static_cast<std::streamoff>(offset) * SECTOR_SIZE);

	// Read 4-byte length
	uint32_t length = 0;
	file.read(reinterpret_cast<char*>(&length), 4);
	if (file.fail())
		return nullptr;
	length = __builtin_bswap32(length);

	// Guard against corrupt/zero length before subtracting
	if (length < 1) {
		GlobalLogger().warn << "Invalid chunk length: " << length << "\n";
		return nullptr;
	}

	// Read compression type
	uint8_t format = 0;
	file.read(reinterpret_cast<char*>(&format), 1);
	if (file.fail())
		return nullptr;

	if (format != REGION_ZLIB) {
		GlobalLogger().warn << "Unsupported compression format: " << int(format) << "\n";
		return nullptr;
	}

	// Read compressed data (length includes the format byte, so actual data is length-1)
	std::vector<uint8_t> compressed(length - 1);
	file.read(reinterpret_cast<char*>(compressed.data()), length - 1);
	if (file.fail())
		return nullptr;

	return DecodeNbtData(compressed);
}

std::vector<uint8_t> Region::EncodeNbtData(const std::shared_ptr<Chunk>& _chunk, int64_t _timestamp,
                                           std::shared_ptr<const std::vector<Tag>> _entities) {
	// Build a compound tag representing a chunk level entry
	Tag root;
	root.type = TAG_COMPOUND;
	root.name = "";

	Tag level;
	level.type = TAG_COMPOUND;
	level.name = "Level";

	// Scalar values
	Tag xPos;
	xPos.type = TAG_INT;
	xPos.name = "xPos";
	xPos.intValue = _chunk->cpos.x;
	Tag zPos;
	zPos.type = TAG_INT;
	zPos.name = "zPos";
	zPos.intValue = _chunk->cpos.z;
	Tag populated;
	populated.type = TAG_BYTE;
	populated.name = "TerrainPopulated";
	populated.byteValue = _chunk->isTerrainPopulated;
	Tag lastUpdate;
	lastUpdate.type = TAG_LONG;
	lastUpdate.name = "LastUpdate";
	lastUpdate.longValue = _timestamp;

	// Byte array, blocks
	Tag blocks;
	blocks.type = TAG_BYTEARRAY;
	blocks.name = "Blocks";
	blocks.byteArray.resize((CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_HEIGHT), 0);

	// Nibble arrays (4 bits per block, so half the size of blocks array)
	Tag data;
	data.type = TAG_BYTEARRAY;
	data.name = "Data";
	data.byteArray.resize((CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_HEIGHT) / 2, 0);

	Tag blockLight;
	blockLight.type = TAG_BYTEARRAY;
	blockLight.name = "BlockLight";
	blockLight.byteArray.resize((CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_HEIGHT) / 2, 0);

	Tag skyLight;
	skyLight.type = TAG_BYTEARRAY;
	skyLight.name = "SkyLight";
	skyLight.byteArray.resize((CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_HEIGHT) / 2, 0);

	// HeightMap, one byte per (x,z) column
	Tag heightMap;
	heightMap.type = TAG_BYTEARRAY;
	heightMap.name = "HeightMap";
	heightMap.byteArray.resize(CHUNK_WIDTH * CHUNK_WIDTH, 0);
	std::copy(std::begin(_chunk->heightMap), std::end(_chunk->heightMap), heightMap.byteArray.data());

	// Put blocks in there
	for (int x = 0; x < CHUNK_WIDTH; x++) {
		for (int z = 0; z < CHUNK_WIDTH; z++) {
			for (int y = 0; y < CHUNK_HEIGHT; y++) {
				size_t idx = size_t(y + (z * CHUNK_HEIGHT) + (x * CHUNK_HEIGHT * CHUNK_WIDTH));
				blocks.byteArray[idx] = _chunk->GetBlock({ x, y, z });
				if (y % 2 == 0) {
					data.byteArray[idx / 2] |= (_chunk->GetMeta({ x, y, z }));
					skyLight.byteArray[idx / 2] |= _chunk->GetSkyLight({ x, y, z });
					blockLight.byteArray[idx / 2] |= _chunk->GetBlockLight({ x, y, z });
				} else {
					data.byteArray[idx / 2] |= (_chunk->GetMeta({ x, y, z }) << 4);
					skyLight.byteArray[idx / 2] |= (_chunk->GetSkyLight({ x, y, z }) << 4);
					blockLight.byteArray[idx / 2] |= (_chunk->GetBlockLight({ x, y, z }) << 4);
				}
			}
		}
	}

	// List tag for entities
	Tag entitiesTag;
	entitiesTag.type = TAG_LIST;
	entitiesTag.name = "Entities";
	entitiesTag.listType = TAG_COMPOUND;
	if (_entities) {
		entitiesTag.list.insert(entitiesTag.list.end(), _entities->begin(), _entities->end());
	}

	// Nested compound inside a list for tile entities
	Tag tileEntities;
	tileEntities.type = TAG_LIST;
	tileEntities.name = "TileEntities";
	tileEntities.listType = TAG_COMPOUND;
	for (auto& te : _chunk->tileEntities) {
		if (te)
			tileEntities.list.push_back(te->Serialize());
	}

	// Assemble level compound
	level.compound["xPos"] = xPos;
	level.compound["zPos"] = zPos;
	level.compound["TerrainPopulated"] = populated;
	level.compound["LastUpdate"] = lastUpdate;
	level.compound["Blocks"] = blocks;
	level.compound["Data"] = data;
	level.compound["BlockLight"] = blockLight;
	level.compound["SkyLight"] = skyLight;
	level.compound["HeightMap"] = heightMap;
	level.compound["Entities"] = entitiesTag;
	level.compound["TileEntities"] = tileEntities;

	root.compound["Level"] = level;

	// Serialize to bytes
	std::vector<uint8_t> raw;
	NBTwriter writer(raw, root);
	// Compress
	libdeflate_compressor* compressor = libdeflate_alloc_compressor(6);
	if (!compressor)
		return {};
	size_t maxSize = libdeflate_zlib_compress_bound(compressor, static_cast<size_t>(raw.size()));
	std::vector<uint8_t> compressed(maxSize);
	size_t actualSize = libdeflate_zlib_compress(compressor, raw.data(), static_cast<size_t>(raw.size()),
	                                             compressed.data(), maxSize);
	libdeflate_free_compressor(compressor);
	compressed.resize(actualSize);
	return compressed;
}

std::shared_ptr<Chunk> Region::DecodeNbtData(const std::vector<uint8_t>& _rawData) {
	libdeflate_decompressor* decompressor = libdeflate_alloc_decompressor();
	if (!decompressor)
		return std::make_shared<Chunk>();
	size_t decompressedSize = (16 * 128 * 16) * 2.5;
	std::vector<uint8_t> decompressed(decompressedSize);
	while (true) {
		decompressed.resize(decompressedSize);
		size_t actualOutSize = 0;
		libdeflate_result result = libdeflate_zlib_decompress(decompressor, _rawData.data(), _rawData.size(),
		                                                      decompressed.data(), decompressed.size(), &actualOutSize);

		if (result == LIBDEFLATE_SUCCESS) {
			decompressed.resize(actualOutSize);
			break;
		}

		if (result != LIBDEFLATE_INSUFFICIENT_SPACE) {
			GlobalLogger().warn << "Decompression failed!\n";
			break;
		}

		decompressedSize *= 1.5;
	}
	libdeflate_free_decompressor(decompressor);
	NBTParser parser(decompressed.data(), int64_t(decompressed.size()));
	const Tag& lvl = parser.root.Get("Level");

	int32_t cx = lvl.Get("xPos").GetInt();
	int32_t cz = lvl.Get("zPos").GetInt();
	bool tp = lvl.Get("TerrainPopulated").GetByte() != 0;
	[[maybe_unused]] int64_t lu = lvl.Get("LastUpdate").GetLong();

	const auto& blocks = lvl.Get("Blocks").GetByteArray();
	const auto& data = lvl.Get("Data").GetByteArray();
	const auto& blockLight = lvl.Get("BlockLight").GetByteArray();
	const auto& skyLight = lvl.Get("SkyLight").GetByteArray();
	const auto& heightMap = lvl.Get("HeightMap").GetByteArray();
	const auto& tileEntities = lvl.Get("TileEntities").GetList();
	const auto& entities = lvl.Get("Entities").GetList();

	// Setup our chunk
	auto chunk = std::make_shared<Chunk>();
	chunk->cpos = Int32_2{ cx, cz };
	chunk->state = tp ? ChunkState::Populated : ChunkState::Generated;
	chunk->isTerrainPopulated = tp;
	std::copy(heightMap.begin(), heightMap.end(), chunk->heightMap);

	// Load all of our block data
	for (int y = 0; y < CHUNK_HEIGHT; y++) {
		for (int x = 0; x < CHUNK_WIDTH; x++) {
			for (int z = 0; z < CHUNK_WIDTH; z++) {
				size_t idx = size_t(y + (z * CHUNK_HEIGHT) + (x * CHUNK_HEIGHT * CHUNK_WIDTH));
				chunk->SetBlock({ x, y, z }, BlockType(blocks[idx]));
				if (y % 2 == 0) {
					chunk->SetMeta({ x, y, z }, data[idx / 2] & 0xF);
					chunk->SetBlockLight({ x, y, z }, blockLight[idx / 2] & 0xF);
					chunk->SetSkyLight({ x, y, z }, skyLight[idx / 2] & 0xF);
				} else {
					chunk->SetMeta({ x, y, z }, (data[idx / 2] >> 4) & 0xF);
					chunk->SetBlockLight({ x, y, z }, (blockLight[idx / 2] >> 4) & 0xF);
					chunk->SetSkyLight({ x, y, z }, (skyLight[idx / 2] >> 4) & 0xF);
				}
			}
		}
	}

	// Load our entities
	chunk->entityTags = std::move(entities);

	// Load our tile entities
	for (auto& te : tileEntities) {
		const auto& id = te.Get("id").GetString();
		int32_t tx = te.Get("x").GetInt();
		int32_t ty = te.Get("y").GetInt();
		int32_t tz = te.Get("z").GetInt();
		Int3 pos{ tx, ty, tz };

		// load a standard slot-based inventory from an Items list tag
		auto loadSlots = [&](auto& _slots) {
			if (!te.Has("Items"))
				return;
			for (auto& item : te.Get("Items").GetList()) {
				int8_t slot = item.Get("Slot").GetByte();
				int16_t itemId = item.Get("id").GetShort();
				int8_t count = item.Get("Count").GetByte();
				int16_t damage = item.Get("Damage").GetShort();
				if (slot >= 0 && slot < (int8_t)_slots.size()) {
					_slots[size_t(slot)] = ItemStack{ itemId, count, damage };
				}
			}
		};

		if (id == "Chest") {
			auto ent = std::make_shared<TileEntityChest>(pos);
			loadSlots(ent->inventory.slots);
			chunk->tileEntities.push_back(std::move(ent));
		} else if (id == "Furnace") {
			auto ent = std::make_shared<TileEntityFurnace>(pos);
			loadSlots(ent->inventory.slots);
			chunk->tileEntities.push_back(std::move(ent));
		} else if (id == "Trap") {
			auto ent = std::make_shared<TileEntityDispenser>(pos);
			loadSlots(ent->inventory.slots);
			chunk->tileEntities.push_back(std::move(ent));
		} else if (id == "Sign") {
			auto ent = std::make_shared<TileEntitySign>(pos);
			if (te.Has("Text1"))
				ent->text1 = te.Get("Text1").GetString();
			if (te.Has("Text2"))
				ent->text2 = te.Get("Text2").GetString();
			if (te.Has("Text3"))
				ent->text3 = te.Get("Text3").GetString();
			if (te.Has("Text4"))
				ent->text4 = te.Get("Text4").GetString();
			chunk->tileEntities.push_back(std::move(ent));
		} else if (id == "MobSpawner") {
			auto ent = std::make_shared<TileEntityMobSpawner>(pos);
			if (te.Has("EntityId"))
				ent->entityId = te.Get("EntityId").GetString();
			if (te.Has("Delay"))
				ent->delay = te.Get("Delay").GetShort();
			chunk->tileEntities.push_back(std::move(ent));
		}
	}

	return chunk;
}