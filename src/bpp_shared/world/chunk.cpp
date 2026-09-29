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
			subChunkFillLight[size_t(i)].skyLight = 15;
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
	subChunkFillBlock.fill(BLOCK_AIR);
	subChunkFillLight.fill(CombinedLight{});
	std::memset(heightMap, 0, sizeof(heightMap));
	std::memset(temperature, 0, sizeof(temperature));
	std::memset(humidity, 0, sizeof(humidity));
}

SubChunk& Chunk::CreateSubChunk(int _index) {
	auto& slot = subChunks[size_t(_index)];

	if (!slot) {
		slot = std::make_unique<SubChunk>();

		if (subChunkFillBlock[size_t(_index)] != BLOCK_AIR)
			std::fill(
				std::begin(slot->blocks),
				std::end(slot->blocks),
				subChunkFillBlock[size_t(_index)]
			);

		if (subChunkFillLight[size_t(_index)].skyLight != 0)
			slot->FillSkyLight(subChunkFillLight[size_t(_index)].skyLight);
		if (subChunkFillLight[size_t(_index)].blockLight != 0)
    		slot->FillBlockLight(subChunkFillLight[size_t(_index)].blockLight);
	}

	return *slot;
}

bool Chunk::CompactSubChunk(int _index) {
	const SubChunk* sub = subChunks[size_t(_index)].get();
	if (!sub)
		return false;
	bool allMatchingBlock = true;
	const BlockType previousBlock = sub->blocks[0];

	const uint8_t light = uint8_t(sub->lightNibble[0]);
	for (int i = 0; i < SubChunk::VOLUME; i++) {
		if (sub->blocks[i] != previousBlock)
			allMatchingBlock = false;

		// Block and sky light must match everywhere
		if (sub->lightNibble[i] != light)
			return false;
	}

	for (int i = 0; i < SubChunk::META_VOLUME; i++) {
		if (sub->nibbleBlockMeta[i] != 0)
			return false;
	}

	if (!allMatchingBlock)
		return false;

	subChunkFillBlock[size_t(_index)] = previousBlock;
	subChunkFillLight[size_t(_index)].skyLight = light >> 4;
	subChunkFillLight[size_t(_index)].blockLight = light & 0xF;
	subChunks[size_t(_index)].reset();
	return true;
}

void Chunk::Compact() {
	for (int i = 0; i < SUB_CHUNK_COUNT; i++)
		CompactSubChunk(i);
}

void Chunk::CopyStorageFrom(const Chunk& _other) {
	for (size_t i = 0; i < subChunks.size(); i++) {
		if (_other.subChunks[i])
			subChunks[i] = std::make_unique<SubChunk>(*_other.subChunks[i]);
		else
			subChunks[i].reset();
	}
	subChunkFillLight = _other.subChunkFillLight;
	subChunkFillBlock = _other.subChunkFillBlock;
}
