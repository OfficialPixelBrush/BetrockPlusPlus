/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "chunk.h"
#include <algorithm>

int Chunk::GetHighestPoint() const {
	int highestPoint = 0;
	for (auto& i : heightMap) {
		if (i > highestPoint)
			highestPoint = i;
	}
	return highestPoint;
}

bool Chunk::CanBlockSeeSky(Int3 _pos) const {
	return _pos.y >= GetHeightValue({ _pos.x, _pos.z });
}

void Chunk::GenerateHeightMap() {
	for (int x = 0; x < CHUNK_WIDTH; x++)
		for (int z = 0; z < CHUNK_WIDTH; z++)
			GenerateHeightMapColumn({ x, z });
}

void Chunk::GenerateHeightMapColumn(Int2 _pos) {
	for (int y = CHUNK_HEIGHT - 1; y >= 0; y--) {
		if (Blocks::blockProperties[GetBlock({ _pos.x, y, _pos.z })].lightOpacity > 0) {
			SetHeightValue(_pos, uint8_t(y + 1));
			return;
		}
	}
	SetHeightValue(_pos, 0);
}

void Chunk::GenerateSkylightMap() {
	GenerateHeightMap();

	// Every slot that starts at or above the tallest column is open sky in every column, so light it in one go
	const int bulkStart = CrossPlatform::Math::Min(CHUNK_HEIGHT, (GetHighestPoint() + SUB_CHUNK_SIZE - 1) & ~(SUB_CHUNK_SIZE - 1));
	
	for (int i = bulkStart / SUB_CHUNK_SIZE; i < SUB_CHUNK_COUNT; i++)
		subChunks[size_t(i)].FillSkyLight(15);

	for (int x = 0; x < CHUNK_WIDTH; x++) {
		for (int z = 0; z < CHUNK_WIDTH; z++) {
			int height = GetHeightValue({ x, z });
			for (int y = bulkStart - 1; y >= height; y--)
				SetSkyLight({ x, y, z }, 15);
			int skyLight = 15;
			for (int y = height - 1; y >= 0; y--) {
				skyLight -= CrossPlatform::Math::Max(1,
				                                     int(Blocks::blockProperties[GetBlock({ x, y, z })].lightOpacity));
				skyLight = CrossPlatform::Math::Max(0, skyLight);
				SetSkyLight({ x, y, z }, uint8_t(skyLight));
			}
		}
	}
	isModified = true;
	Compact();
}

void Chunk::RelightColumn(Int2 _pos) {
	GenerateHeightMapColumn(_pos);
	int height = GetHeightValue(_pos);

	for (int y = CHUNK_HEIGHT - 1; y >= height; y--)
		SetSkyLight({ _pos.x, y, _pos.z }, 15);
}

void Chunk::Clear() {
	isTerrainPopulated = false;
	isModified = false;
	climateBaked = false;
	for (auto& sub : subChunks)
		sub.Reset();
	std::memset(heightMap, 0, sizeof(heightMap));
	if (!chunkClimate)
		return;
    chunkClimate.reset();
}

namespace {
std::unique_ptr<SubChunk::NibbleLayer> MakeNibbleLayer(uint8_t _fill) {
	// Every byte is about to be overwritten, so skip the zeroing
	auto layer = std::make_unique_for_overwrite<SubChunk::NibbleLayer>();
	layer->fill(uint8_t(((_fill & 0x0Fu) << 4) | (_fill & 0x0Fu)));
	return layer;
}

// A nibble layer is uniform when the first byte has matching halves and every other byte equals it
bool CompactNibbleLayer(std::unique_ptr<SubChunk::NibbleLayer>& _layer, uint8_t& _fill) {
	if (!_layer)
		return false;

	// Most layers are mixed, so this usually exits within a few voxels
	const uint8_t first = (*_layer)[0];
	if ((first >> 4) != (first & 0x0Fu))
		return false;
	for (size_t i = 1; i < SubChunk::NIBBLE_BYTES; i++) {
		if ((*_layer)[i] != first)
			return false;
	}

	_fill = first & 0x0Fu;
	_layer.reset();
	return true;
}

template <typename T>
std::unique_ptr<T> CopyLayer(const std::unique_ptr<T>& _src) {
	return _src ? std::make_unique<T>(*_src) : nullptr;
}
} // namespace

void SubChunk::AllocBlocks() {
	blocks = std::make_unique_for_overwrite<BlockLayer>();
	blocks->fill(blockFill);
}

void SubChunk::AllocMeta() {
	meta = MakeNibbleLayer(metaFill);
}

void SubChunk::AllocBlockLight() {
	blockLight = MakeNibbleLayer(blockLightFill);
}

void SubChunk::AllocSkyLight() {
	skyLight = MakeNibbleLayer(skyLightFill);
}

bool SubChunk::CompactBlocks() {
	if (!blocks)
		return false;

	const BlockType first = (*blocks)[0];
	for (size_t i = 1; i < size_t(VOLUME); i++) {
		if ((*blocks)[i] != first)
			return false;
	}

	blockFill = first;
	blocks.reset();
	return true;
}

bool SubChunk::CompactMeta() {
	return CompactNibbleLayer(meta, metaFill);
}

bool SubChunk::CompactBlockLight() {
	return CompactNibbleLayer(blockLight, blockLightFill);
}

bool SubChunk::CompactSkyLight() {
	return CompactNibbleLayer(skyLight, skyLightFill);
}

bool SubChunk::Compact() {
	const bool freedBlocks = CompactBlocks();
	const bool freedMeta = CompactMeta();
	const bool freedBlockLight = CompactBlockLight();
	const bool freedSkyLight = CompactSkyLight();
	return freedBlocks || freedMeta || freedBlockLight || freedSkyLight;
}

void SubChunk::Reset() {
	blocks.reset();
	meta.reset();
	blockLight.reset();
	skyLight.reset();
	blockFill = BLOCK_AIR;
	metaFill = 0;
	blockLightFill = 0;
	skyLightFill = 15;
}

void SubChunk::CopyFrom(const SubChunk& _other) {
	blocks = CopyLayer(_other.blocks);
	meta = CopyLayer(_other.meta);
	blockLight = CopyLayer(_other.blockLight);
	skyLight = CopyLayer(_other.skyLight);
	blockFill = _other.blockFill;
	metaFill = _other.metaFill;
	blockLightFill = _other.blockLightFill;
	skyLightFill = _other.skyLightFill;
}

bool Chunk::TryToCompactSubChunk(int _index) {
	assert(_index >= 0 && _index < SUB_CHUNK_COUNT);
	return subChunks[size_t(_index)].Compact();
}

void Chunk::Compact() {
	for (auto& sub : subChunks)
		sub.Compact();
}

void Chunk::CopyStorageFrom(const Chunk& _other) {
	for (size_t i = 0; i < subChunks.size(); i++)
		subChunks[i].CopyFrom(_other.subChunks[i]);
}
