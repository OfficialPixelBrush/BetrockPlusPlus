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
	const int bulkStart = CrossPlatform::Math::Min(CHUNK_HEIGHT,
	                                               (GetHighestPoint() + SUB_CHUNK_SIZE - 1) & ~(SUB_CHUNK_SIZE - 1));

	FillSkyLightSlices(bulkStart / SUB_CHUNK_SIZE, 15);

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
	// Flat chunks stay flat until they've settled, see ConvertToSubChunks()
	Compact();
}

void Chunk::RelightColumn(Int2 _pos) {
	GenerateHeightMapColumn(_pos);
	int height = GetHeightValue(_pos);

	for (int y = CHUNK_HEIGHT - 1; y >= height; y--)
		SetSkyLight({ _pos.x, y, _pos.z }, 15);
}

void Chunk::RecalculateSkyLightColumn(Int2 _pos) {
	GenerateHeightMapColumn(_pos);
	const int height = GetHeightValue(_pos);

	for (int y = CHUNK_HEIGHT - 1; y >= height; y--)
		SetSkyLight({ _pos.x, y, _pos.z }, 15);

	int skyLight = 15;
	for (int y = height - 1; y >= 0; y--) {
		skyLight -= CrossPlatform::Math::Max(1,
		                                     int(Blocks::blockProperties[GetBlock({ _pos.x, y, _pos.z })].lightOpacity));
		skyLight = CrossPlatform::Math::Max(0, skyLight);
		SetSkyLight({ _pos.x, y, _pos.z }, uint8_t(skyLight));
	}
}

void Chunk::Clear() {
	isTerrainPopulated = false;
	isModified = false;
	climateBaked = false;
	if (flat)
		flat->Reset();
	for (auto& sub : subChunks)
		sub.Reset();
	std::memset(heightMap, 0, sizeof(heightMap));
	std::memset(temperature, 0, sizeof(temperature));
	std::memset(humidity, 0, sizeof(humidity));
}

namespace {
std::unique_ptr<SubChunk::NibbleLayer> MakeNibbleLayer(uint8_t _fill) {
	// Every byte is about to be overwritten, so skip the zeroing
	auto layer = std::make_unique_for_overwrite<SubChunk::NibbleLayer>();
	const uint8_t packed = uint8_t(((_fill & 0xF) << 4) | (_fill & 0xF));
	layer->data.fill(packed);
	return layer;
}

// A nibble layer is uniform when the first byte has matching halves and every other byte equals it
bool CompactNibbleLayer(std::unique_ptr<SubChunk::NibbleLayer>& _layer, uint8_t& _fill) {
	if (!_layer)
		return false;

	const uint8_t first = _layer->Get(0);
	for (size_t i = 1; i < SubChunk::VOLUME; i++) {
		if (_layer->Get(i) != first)
			return false;
	}

	_fill = first;
	_layer.reset();
	return true;
}

template <typename T>
std::unique_ptr<T> CopyLayer(const std::unique_ptr<T>& _src) {
	return _src ? std::make_unique<T>(*_src) : nullptr;
}
} // namespace

namespace {
// Fills one slice's blocks from flat storage. A slice that's all one block type just gets a fill
void CompactSliceBlocks(const FlatChunk& _g, int _slice, SubChunk& _sub) {
	const BlockType* types = _g.block.data() + size_t(_slice) * SubChunk::VOLUME;
	const BlockType first = types[0];
	if (std::all_of(types + 1, types + SubChunk::VOLUME, [first](BlockType _b) { return _b == first; })) {
		_sub.blockFill = first;
		return;
	}
	_sub.blocks = std::make_unique_for_overwrite<SubChunk::BlockLayer>();
	std::copy_n(types, SubChunk::VOLUME, _sub.blocks->types.begin());
	_sub.blocks->counts.fill(0);
	for (BlockType b : _sub.blocks->types)
		++_sub.blocks->counts[size_t(b)];
}

// Same for one of the nibble arrays. Both layouts store two nibbles per byte, so a slice of the flat
// array is a ready made layer. Returns true if the slice is uniform, with its value in _fill.
// Otherwise allocates _layer and returns false.
bool CompactSliceNibbles(const FlatChunk::NibbleArray& _src, int _slice, std::unique_ptr<SubChunk::NibbleLayer>& _layer,
                         uint8_t& _fill) {
	constexpr size_t BYTES = SubChunk::VOLUME / 2;
	const uint8_t* bytes = _src.data.data() + size_t(_slice) * BYTES;
	const uint8_t first = bytes[0];
	if ((first & 0x0Fu) == (first >> 4) &&
	    std::all_of(bytes + 1, bytes + BYTES, [first](uint8_t _b) { return _b == first; })) {
		_fill = uint8_t(first & 0x0Fu);
		return true;
	}
	_layer = std::make_unique_for_overwrite<SubChunk::NibbleLayer>();
	std::copy_n(bytes, BYTES, _layer->data.begin());
	return false;
}

// Compacts flat storage into _out, overwriting whatever was there. Doesn't touch _g
void CompactFlatInto(const FlatChunk& _g, std::array<SubChunk, SUB_CHUNK_COUNT>& _out) {
	for (int s = 0; s < SUB_CHUNK_COUNT; s++) {
		SubChunk& sub = _out[size_t(s)];
		sub.Reset();
		CompactSliceBlocks(_g, s, sub);
		uint8_t fill = 0;
		if (CompactSliceNibbles(_g.meta, s, sub.meta, fill))
			sub.metaFill = fill;
		if (CompactSliceNibbles(_g.blockLight, s, sub.blockLight, fill))
			sub.blockLightFill = fill;
		if (CompactSliceNibbles(_g.skyLight, s, sub.skyLight, fill))
			sub.skyLightFill = fill;
	}
}
} // namespace

void Chunk::AllocGenStorage() {
	// Nothing in subchunks matters once we're flat, make sure nothing stale is hanging around
	for (auto& sub : subChunks)
		sub.Reset();
	flat = std::make_shared<FlatChunk>();
}

void Chunk::ConvertToSubChunks() {
	if (!flat)
		return;
	// Fill the subchunks before dropping the arrays, so there's never a moment with neither
	CompactFlatInto(*flat, subChunks);
	flat.reset();
}

void Chunk::FillSkyLightSlices(int _firstSlice, uint8_t _val) {
	_firstSlice = CrossPlatform::Math::Max(_firstSlice, 0);
	if (_firstSlice >= SUB_CHUNK_COUNT)
		return;
	if (flat) {
		FlatChunk::FillNibbleSpan(flat->skyLight, size_t(_firstSlice) * SubChunk::VOLUME,
		                         size_t(SUB_CHUNK_COUNT - _firstSlice) * SubChunk::VOLUME, _val);
		return;
	}
	for (int i = _firstSlice; i < SUB_CHUNK_COUNT; i++)
		subChunks[size_t(i)].FillSkyLight(_val);
}

void SubChunk::AllocBlocks() {
	blocks = std::make_unique_for_overwrite<BlockLayer>();
	blocks->types.fill(blockFill);
	blocks->counts.fill(0);
	blocks->counts[size_t(blockFill)] = uint16_t(VOLUME);
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

void SubChunk::FillRegion(Int3 _a, Int3 _b, BlockType _type, uint8_t _meta) {
	_meta &= 0xF;
	const bool fullFootprint = _a.x == 0 && _b.x == SIZE - 1 && _a.z == 0 && _b.z == SIZE - 1;

	if (fullFootprint && _a.y == 0 && _b.y == SIZE - 1) {
		FillBlocks(_type);
		FillMeta(_meta);
		return;
	}

	if (blocks || _type != blockFill) {
		if (!blocks)
			AllocBlocks();
		if (fullFootprint) {
			FillBlockSpan(size_t(_a.y) * SIZE * SIZE, size_t(_b.y - _a.y + 1) * SIZE * SIZE, _type);
		} else {
			for (int y = _a.y; y <= _b.y; y++)
				for (int z = _a.z; z <= _b.z; z++)
					FillBlockSpan(size_t(LocalIndex({ _a.x, y, z })), size_t(_b.x - _a.x + 1), _type);
		}
	}

	if (meta || _meta != metaFill) {
		if (!meta)
			AllocMeta();
		if (fullFootprint) {
			const uint8_t packed = uint8_t(_meta | (_meta << 4));
			std::memset(meta->data.data() + size_t(_a.y) * SIZE * SIZE / 2, packed,
            	size_t(_b.y - _a.y + 1) * SIZE * SIZE / 2);
		} else {
			for (int y = _a.y; y <= _b.y; y++)
				for (int z = _a.z; z <= _b.z; z++)
					for (int x = _a.x; x <= _b.x; x++)
						meta->Set(LocalIndex({ x, y, z }), _meta);
		}
	}
}

namespace {
// Normalizes and clamps a chunk-local box. Returns false if nothing of it is inside the chunk.
bool ClampChunkBox(Int3 _a, Int3 _b, Int3& _lo, Int3& _hi) {
	const int loX = CrossPlatform::Math::Min(_a.x, _b.x), hiX = CrossPlatform::Math::Max(_a.x, _b.x);
	const int loY = CrossPlatform::Math::Min(_a.y, _b.y), hiY = CrossPlatform::Math::Max(_a.y, _b.y);
	const int loZ = CrossPlatform::Math::Min(_a.z, _b.z), hiZ = CrossPlatform::Math::Max(_a.z, _b.z);
	if (hiX < 0 || loX >= CHUNK_WIDTH || hiY < 0 || loY >= CHUNK_HEIGHT || hiZ < 0 || loZ >= CHUNK_WIDTH)
		return false;
	_lo = { CrossPlatform::Math::Max(loX, 0), CrossPlatform::Math::Max(loY, 0), CrossPlatform::Math::Max(loZ, 0) };
	_hi = { CrossPlatform::Math::Min(hiX, CHUNK_WIDTH - 1), CrossPlatform::Math::Min(hiY, CHUNK_HEIGHT - 1),
		    CrossPlatform::Math::Min(hiZ, CHUNK_WIDTH - 1) };
	return true;
}

// Calls _f(startIndex, count) for every contiguous run of voxels in a chunk-local box.
// A box covering whole slices is a single run.
template <typename F>
void ForEachFlatRun(Int3 _lo, Int3 _hi, F&& _f) {
	if (_lo.x == 0 && _hi.x == CHUNK_WIDTH - 1 && _lo.z == 0 && _hi.z == CHUNK_WIDTH - 1) {
		_f(size_t(FlatChunk::Index({ 0, _lo.y, 0 })), size_t(_hi.y - _lo.y + 1) * CHUNK_AREA);
		return;
	}
	for (int y = _lo.y; y <= _hi.y; y++)
		for (int z = _lo.z; z <= _hi.z; z++)
			_f(size_t(FlatChunk::Index({ _lo.x, y, z })), size_t(_hi.x - _lo.x + 1));
}
} // namespace

void Chunk::FillRegion(Int3 _a, Int3 _b, BlockType _type, uint8_t _meta) {
	Int3 lo, hi;
	if (!ClampChunkBox(_a, _b, lo, hi))
		return;
	if (flat) {
		ForEachFlatRun(lo, hi, [&](size_t _start, size_t _count) {
			flat->FillBlockSpan(_start, _count, _type);
			FlatChunk::FillNibbleSpan(flat->meta, _start, _count, _meta);
		});
		isModified = true;
		return;
	}
	for (int sub = lo.y >> 4; sub <= (hi.y >> 4); sub++) {
		const int base = sub * SUB_CHUNK_SIZE;
		const int y0 = CrossPlatform::Math::Max(lo.y, base) - base;
		const int y1 = CrossPlatform::Math::Min(hi.y, base + SUB_CHUNK_SIZE - 1) - base;
		subChunks[size_t(sub)].FillRegion({ lo.x, y0, lo.z }, { hi.x, y1, hi.z }, _type, _meta);
	}
	isModified = true;
}

void Chunk::FillBlockLightRegion(Int3 _a, Int3 _b, uint8_t _val) {
	Int3 lo, hi;
	if (!ClampChunkBox(_a, _b, lo, hi))
		return;
	_val &= 0xF;
	if (flat) {
		ForEachFlatRun(lo, hi, [&](size_t _start, size_t _count) {
			FlatChunk::FillNibbleSpan(flat->blockLight, _start, _count, _val);
		});
		isModified = true;
		return;
	}
	const bool fullFootprint = lo.x == 0 && hi.x == CHUNK_WIDTH - 1 && lo.z == 0 && hi.z == CHUNK_WIDTH - 1;
	for (int sub = lo.y >> 4; sub <= (hi.y >> 4); sub++) {
		const int base = sub * SUB_CHUNK_SIZE;
		const int y0 = CrossPlatform::Math::Max(lo.y, base) - base;
		const int y1 = CrossPlatform::Math::Min(hi.y, base + SUB_CHUNK_SIZE - 1) - base;
		SubChunk& s = subChunks[size_t(sub)];
		if (fullFootprint && y0 == 0 && y1 == SUB_CHUNK_SIZE - 1) {
			s.FillBlockLight(_val);
			continue;
		}
		for (int y = y0; y <= y1; y++)
			for (int z = lo.z; z <= hi.z; z++)
				for (int x = lo.x; x <= hi.x; x++)
					s.SetBlockLight(SubChunk::LocalIndex({ x, y, z }), _val);
	}
	isModified = true;
}

bool SubChunk::CompactBlocks() {
	if (!blocks)
		return false;

	const BlockType first = blocks->types[0];
	if (blocks->counts[size_t(first)] != VOLUME)
		return false;

	blockFill = first;
	blocks.reset();
	return true;
}

bool SubChunk::CompactMeta() {
	uint8_t fill;
	if (!CompactNibbleLayer(meta, fill))
		return false;
	metaFill = fill;
	return true;
}

bool SubChunk::CompactBlockLight() {
	uint8_t fill;
	if (!CompactNibbleLayer(blockLight, fill))
		return false;
	blockLightFill = fill;
	return true;
}

bool SubChunk::CompactSkyLight() {
	uint8_t fill;
	if (!CompactNibbleLayer(skyLight, fill))
		return false;
	skyLightFill = fill;
	return true;
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
	if (flat)
		return false;
	return subChunks[size_t(_index)].Compact();
}

void Chunk::Compact() {
	if (flat)
		return;
	for (auto& sub : subChunks)
		sub.Compact();
}

void Chunk::CopyStorageFrom(const Chunk& _other) {
	// Copies are always compact, that's what gets saved
	flat.reset();
	if (_other.flat) {
		CompactFlatInto(*_other.flat, subChunks);
		return;
	}
	for (size_t i = 0; i < subChunks.size(); i++)
		subChunks[i].CopyFrom(_other.subChunks[i]);
}
