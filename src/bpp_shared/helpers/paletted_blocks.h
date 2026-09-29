/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once
#include "enums/blocks.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <utility>

// Palette compressed storage for a fixed run of blocks (one SubChunk's worth).
//
// Storage comes in tiers, chosen by how many distinct block types are in use:
//
//   bits | distinct types | index buffer   | notes
//   -----+----------------+----------------+---------------------------------------
//     0  |       1        | none           | uniform, the block is just palette[0]
//     1  |      2         | Count / 8      | indices into the palette
//     2  |     3-4        | Count / 4      | indices into the palette
//     4  |     5-16       | Count / 2      | indices into the palette
//     8  |     17+        | Count          | direct, the buffer holds the block ids
//
// Every tier that isn't direct keeps entries byte aligned (1, 2 and 4 all divide 8), so a lookup never
// straddles two bytes. The palette grows on demand and never shrinks by itself, since that would need a
// refcount per entry. Call Repack() when it's a good time to pay for a full scan (chunk generation or load
// finishing), it rebuilds using the fewest bits that fit.
//
// Not thread safe. Growing replaces the index buffer, so a reader running alongside a writer can see freed memory.
template <std::size_t Count>
class PalettedBlocks {
	static_assert(Count > 0 && Count % 8 == 0, "Count must be a multiple of 8 so every tier fills whole bytes");

	static constexpr unsigned MAX_PALETTE = 16; // largest palette, which is what 4 bit indices can address
	static constexpr uint8_t DIRECT_BITS = 8;

public:
	PalettedBlocks() = default;

	PalettedBlocks(const PalettedBlocks& _other) : mBits(_other.mBits), mPaletteSize(_other.mPaletteSize) {
		std::memcpy(mPalette, _other.mPalette, sizeof(mPalette));
		if (_other.mData) {
			const std::size_t bytes = DataBytes(mBits);
			mData = std::make_unique<uint8_t[]>(bytes);
			std::memcpy(mData.get(), _other.mData.get(), bytes);
		}
	}
	PalettedBlocks(PalettedBlocks&& _other) noexcept {
		Swap(_other); // leaves _other as a valid, empty (all air) storage
	}
	// Takes by value so it handles both copy and move assignment
	PalettedBlocks& operator=(PalettedBlocks _other) noexcept {
		Swap(_other);
		return *this;
	}

	inline BlockType Get(std::size_t _i) const {
		assert(_i < Count);
		if (mBits == 0)
			return mPalette[0];
		if (mBits == DIRECT_BITS)
			return BlockType(int8_t(mData[_i]));
		return mPalette[ReadIndex(_i)];
	}

	inline void Set(std::size_t _i, BlockType _type) {
		assert(_i < Count);
		if (mBits == DIRECT_BITS) {
			mData[_i] = uint8_t(_type);
			return;
		}

		unsigned entry = Find(_type);
		if (entry == NOT_FOUND) {
			if (mPaletteSize >= CapacityFor(mBits)) {
				Grow();
				if (mBits == DIRECT_BITS) {
					mData[_i] = uint8_t(_type);
					return;
				}
			}
			entry = mPaletteSize;
			mPalette[mPaletteSize++] = _type;
		}

		if (mBits != 0) // a uniform slab has no buffer, and only ever asks for entry 0
			WriteIndex(mData.get(), mBits, _i, entry);
	}

	// Makes every block _type and gives the index buffer back
	inline void Fill(BlockType _type) {
		mData.reset();
		mBits = 0;
		mPaletteSize = 1;
		mPalette[0] = _type;
	}

	// True when every block is the same. Only exact after Repack(), as a palette can hold entries that
	// no longer appear anywhere, but a true result is always correct.
	inline bool IsUniform() const {
		return mBits == 0;
	}
	// The block a uniform slab is made of. Only meaningful when IsUniform()
	inline BlockType UniformType() const {
		return mPalette[0];
	}

	// Rebuilds with the fewest bits that fit what's actually stored, dropping dead palette entries.
	// Costs a full scan, so don't call it per block. Does nothing if already as tight as it can be.
	void Repack() {
		if (mBits == 0)
			return;

		bool seen[256] = {};
		BlockType found[MAX_PALETTE] = {};
		unsigned distinct = 0;
		for (std::size_t i = 0; i < Count; i++) {
			const BlockType block = Get(i);
			const uint8_t key = uint8_t(block);
			if (seen[key])
				continue;
			seen[key] = true;
			if (distinct < MAX_PALETTE)
				found[distinct] = block;
			distinct++;
		}

		const uint8_t newBits = BitsFor(distinct);
		if (newBits == mBits && (mBits == DIRECT_BITS || distinct == mPaletteSize))
			return;

		if (newBits == 0) {
			Fill(found[0]);
			return;
		}

		// newBits is 1, 2 or 4 from here. Direct with more than 16 types returned above, and any other
		// storage can't hold more than 16 types, so the only way to get here is a palette-sized result.
		uint8_t remap[256] = {};
		for (unsigned p = 0; p < distinct; p++)
			remap[uint8_t(found[p])] = uint8_t(p);

		auto data = std::make_unique<uint8_t[]>(DataBytes(newBits));
		for (std::size_t i = 0; i < Count; i++)
			WriteIndex(data.get(), newBits, i, remap[uint8_t(Get(i))]);

		mData = std::move(data);
		mBits = newBits;
		for (unsigned p = 0; p < distinct; p++)
			mPalette[p] = found[p];
		mPaletteSize = uint8_t(distinct);
	}

	// Bytes owned on the heap, not counting sizeof(PalettedBlocks)
	inline std::size_t HeapBytes() const {
		return mData ? DataBytes(mBits) : 0;
	}
	// Bits per block right now, 0 = uniform, 8 = direct
	inline unsigned Bits() const {
		return mBits;
	}
	// Palette entries in use. Meaningless when Bits() is 8, as direct storage has no palette.
	inline unsigned PaletteSize() const {
		return mBits == DIRECT_BITS ? 0 : mPaletteSize;
	}

private:
	static constexpr unsigned NOT_FOUND = ~0u;

	uint8_t mBits = 0;
	uint8_t mPaletteSize = 1;
	BlockType mPalette[MAX_PALETTE] = { BLOCK_AIR };
	std::unique_ptr<uint8_t[]> mData;

	static constexpr unsigned CapacityFor(unsigned _bits) {
		return _bits == 0 ? 1u : (1u << _bits);
	}
	static constexpr uint8_t NextBits(uint8_t _bits) {
		return _bits == 0 ? 1 : (_bits == 1 ? 2 : (_bits == 2 ? 4 : DIRECT_BITS));
	}
	static constexpr uint8_t BitsFor(unsigned _distinct) {
		return _distinct <= 1 ? 0 : (_distinct <= 2 ? 1 : (_distinct <= 4 ? 2 : (_distinct <= MAX_PALETTE ? 4 : DIRECT_BITS)));
	}
	static constexpr std::size_t DataBytes(unsigned _bits) {
		return Count * _bits / 8;
	}

	inline unsigned Find(BlockType _type) const {
		for (unsigned i = 0; i < mPaletteSize; i++) {
			if (mPalette[i] == _type)
				return i;
		}
		return NOT_FOUND;
	}

	inline unsigned ReadIndex(std::size_t _i) const {
		const unsigned bit = unsigned(_i) * mBits;
		return (mData[bit >> 3] >> (bit & 7)) & ((1u << mBits) - 1u);
	}
	static inline void WriteIndex(uint8_t* _data, unsigned _bits, std::size_t _i, unsigned _val) {
		const unsigned bit = unsigned(_i) * _bits;
		const unsigned shift = bit & 7;
		const unsigned mask = ((1u << _bits) - 1u) << shift;
		uint8_t& byte = _data[bit >> 3];
		byte = uint8_t((byte & ~mask) | ((_val << shift) & mask));
	}

	// Moves up one tier, keeping every block as it was. Going direct drops the palette.
	void Grow() {
		const uint8_t newBits = NextBits(mBits);
		auto data = std::make_unique<uint8_t[]>(DataBytes(newBits)); // zeroed

		if (newBits == DIRECT_BITS) {
			for (std::size_t i = 0; i < Count; i++)
				data[i] = uint8_t(Get(i));
		} else if (mBits != 0) { // from uniform every index is already 0
			for (std::size_t i = 0; i < Count; i++)
				WriteIndex(data.get(), newBits, i, ReadIndex(i));
		}

		mData = std::move(data);
		mBits = newBits;
	}

	void Swap(PalettedBlocks& _other) noexcept {
		std::swap(mBits, _other.mBits);
		std::swap(mPaletteSize, _other.mPaletteSize);
		std::swap(mData, _other.mData);
		for (unsigned i = 0; i < MAX_PALETTE; i++)
			std::swap(mPalette[i], _other.mPalette[i]);
	}
};
