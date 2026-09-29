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
	for (int i = bulkStart / SUB_CHUNK_SIZE; i < SUB_CHUNK_COUNT; i++) {
		if (subChunks[size_t(i)])
			subChunks[size_t(i)]->FillSkyLight(15);
		else
			compactSubChunks[size_t(i)].skyLight = 15;
	}

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
		sub.reset();
	compactSubChunks.fill(CompactSubChunk{});
	std::memset(heightMap, 0, sizeof(heightMap));
	std::memset(temperature, 0, sizeof(temperature));
	std::memset(humidity, 0, sizeof(humidity));
}

SubChunk& Chunk::CreateSubChunk(int _index) {
	assert(_index >= 0 && _index < SUB_CHUNK_COUNT);
	auto& slot = subChunks[size_t(_index)];

	if (!slot) {
		slot = std::make_unique<SubChunk>();
		const CompactSubChunk& fill = compactSubChunks[size_t(_index)];

		if (fill.type != BLOCK_AIR)
			slot->blocks.Fill(fill.type);
		if (fill.skyLight != 0)
			slot->FillSkyLight(fill.skyLight);
		if (fill.blockLight != 0)
			slot->FillBlockLight(fill.blockLight);
	}

	return *slot;
}

bool Chunk::TryToCompactSubChunk(int _index) {
	SubChunk* sub = subChunks[size_t(_index)].get();
	if (!sub)
		return false;

	// Shrink the palette first, this is where the memory comes back even when the slab stays allocated.
	// It also makes IsUniform() exact, so no per block comparison is needed below.
	sub->blocks.Repack();
	if (!sub->blocks.IsUniform())
		return false;

	// Bail out on the first mismatch, a lit slab that isn't uniform exits within a few voxels
	const uint8_t firstLight = sub->light[0];
	for (int i = 1; i < SubChunk::VOLUME; i++) {
		if (sub->light[i] != firstLight)
			return false;
	}

	for (int i = 0; i < SubChunk::META_VOLUME; i++) {
		if (sub->nibbleBlockMeta[i] != 0)
			return false;
	}

	CompactSubChunk& fill = compactSubChunks[size_t(_index)];
	fill.type = sub->blocks.UniformType();
	fill.skyLight = firstLight >> 4;
	fill.blockLight = firstLight & 0xF;
	subChunks[size_t(_index)].reset();
	return true;
}

void Chunk::Compact() {
	for (int i = 0; i < SUB_CHUNK_COUNT; i++)
		TryToCompactSubChunk(i);
}

void Chunk::CopyStorageFrom(const Chunk& _other) {
	for (size_t i = 0; i < subChunks.size(); i++) {
		if (_other.subChunks[i])
			subChunks[i] = std::make_unique<SubChunk>(*_other.subChunks[i]);
		else
			subChunks[i].reset();
	}
	compactSubChunks = _other.compactSubChunks;
}
