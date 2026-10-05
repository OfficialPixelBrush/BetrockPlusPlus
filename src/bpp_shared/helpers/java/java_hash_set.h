/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "numeric_structs.h"
#include <cstdint>
#include <vector>

namespace Java {

// java.util.HashMap changed its internals twice, and each version iterates in a different order.
// Java 7 here means 7u6+ with alternative hashing off (the default).
enum class HashMapVersion : uint8_t { Java6, Java7, Java8 };

// Reproduces the iteration order of a java.util.HashSet<ChunkPosition>.
//
// It only tracks the bucket layout, not values, so it is cheap. Like a real HashSet that
// gets clear()ed between uses, the table keeps whatever capacity it has grown to, which
// means the iteration order depends on the largest set ever processed. That is also true
// in vanilla, so one instance should live as long as the Java object it imitates does.
//
// Not emulated: Java 8 treeifies a bucket once it holds 9 entries and the table has at
// least 64 buckets. Tree bins order by System.identityHashCode, so that case isn't
// deterministic in Java either. We keep appending to the list instead.
class ChunkPositionHashSet {
public:
	explicit ChunkPositionHashSet(HashMapVersion _version = HashMapVersion::Java8) : version(_version) {}

	// ChunkPosition.hashCode()
	static int32_t HashCode(const Int3& _p) {
		return int32_t(uint32_t(_p.x) * 8976890u + uint32_t(_p.y) * 981131u + uint32_t(_p.z));
	}

	void SetVersion(HashMapVersion _version) {
		if (_version == version)
			return;
		version = _version;
		table.clear();
		size = 0;
		threshold = 0;
	}

	// HashSet.add
	void Add(const Int3& _key) {
		if (table.empty()) {
			table.resize(DEFAULT_CAPACITY);
			threshold = DEFAULT_CAPACITY * 3 / 4;
		}

		const uint32_t hash = Spread(HashCode(_key));
		if (Contains(_key, hash))
			return;

		switch (version) {
		case HashMapVersion::Java6: {
			// addEntry: insert at the head, then "if (size++ >= threshold) resize(2 * length)"
			auto& bucket = table[hash & (table.size() - 1)];
			bucket.insert(bucket.begin(), Entry{ _key, hash });
			if (size++ >= threshold)
				ResizeJava6And7();
			break;
		}
		case HashMapVersion::Java7: {
			// addEntry: resize first, and only if the target bucket is already occupied
			if (size >= threshold && !table[hash & (table.size() - 1)].empty())
				ResizeJava6And7();
			auto& bucket = table[hash & (table.size() - 1)];
			bucket.insert(bucket.begin(), Entry{ _key, hash });
			size++;
			break;
		}
		case HashMapVersion::Java8: {
			// putVal: append at the tail
			auto& bucket = table[hash & (table.size() - 1)];
			const size_t before = bucket.size();
			bucket.push_back(Entry{ _key, hash });
			// treeifyBin resizes instead of treeifying while the table is small
			if (before >= TREEIFY_THRESHOLD && table.size() < MIN_TREEIFY_CAPACITY)
				ResizeJava8();
			if (++size > threshold)
				ResizeJava8();
			break;
		}
		}
	}

	// new ArrayList(set), followed by set.clear() (which keeps the table size)
	std::vector<Int3> DrainInIterationOrder() {
		std::vector<Int3> result;
		result.reserve(size);
		for (auto& bucket : table) {
			for (auto& entry : bucket)
				result.push_back(entry.key);
			bucket.clear();
		}
		size = 0;
		return result;
	}

	size_t Capacity() const { return table.size(); }

private:
	struct Entry {
		Int3 key;
		uint32_t hash;
	};

	static constexpr size_t DEFAULT_CAPACITY = 16;
	static constexpr size_t TREEIFY_THRESHOLD = 8;
	static constexpr size_t MIN_TREEIFY_CAPACITY = 64;

	HashMapVersion version;
	std::vector<std::vector<Entry>> table;
	size_t size = 0;
	size_t threshold = 0;

	uint32_t Spread(int32_t _h) const {
		uint32_t h = uint32_t(_h);
		if (version == HashMapVersion::Java8)
			return h ^ (h >> 16);
		// Java 6/7 supplemental hash
		h ^= (h >> 20) ^ (h >> 12);
		return h ^ (h >> 7) ^ (h >> 4);
	}

	bool Contains(const Int3& _key, uint32_t _hash) const {
		for (const auto& entry : table[_hash & (table.size() - 1)])
			if (entry.hash == _hash && entry.key == _key)
				return true;
		return false;
	}

	// transfer(): walks each old bucket in order and head-inserts into the new table,
	// which reverses the relative order of entries that land in the same bucket
	void ResizeJava6And7() {
		std::vector<std::vector<Entry>> newTable(table.size() * 2);
		for (auto& bucket : table) {
			for (auto& entry : bucket) {
				auto& dest = newTable[entry.hash & (newTable.size() - 1)];
				dest.insert(dest.begin(), entry);
			}
		}
		table.swap(newTable);
		threshold = table.size() * 3 / 4;
	}

	// resize(): splits each bucket into a "lo" and "hi" list, preserving order
	void ResizeJava8() {
		const size_t oldCap = table.size();
		std::vector<std::vector<Entry>> newTable(oldCap * 2);
		for (size_t j = 0; j < oldCap; j++) {
			for (auto& entry : table[j]) {
				if ((entry.hash & oldCap) == 0)
					newTable[j].push_back(entry);
				else
					newTable[j + oldCap].push_back(entry);
			}
		}
		table.swap(newTable);
		threshold = table.size() * 3 / 4;
	}
};

} // namespace Java
