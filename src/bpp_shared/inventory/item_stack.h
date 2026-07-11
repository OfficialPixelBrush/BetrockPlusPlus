/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "hash.h"
#include "strings/labels.h"
#include <cstdint>
#include <ostream>
#include <sstream>
#include <string>

// Just a virtual container
struct ItemStack {
	ItemId id = ITEM_INVALID;
	ItemAmount count = 0;
	ItemDamage data = 0; // This is "damage" in the og java but data makes more sense for what this is used for

	bool operator==(const ItemStack& o) const {
		return id == o.id && count == o.count && data == o.data;
	}
	bool operator!=(const ItemStack& o) const {
		return !(*this == o);
	}

	friend std::wostream& operator<<(std::wostream& wos, const ItemStack& val) {
		wos << "(" << static_cast<int64_t>(val.id) << ":" << static_cast<int64_t>(val.data) << " x"
		    << static_cast<int64_t>(val.count) << ")";
		return wos;
	}

	friend std::ostream& operator<<(std::ostream& os, const ItemStack& val) {
		os << "(" << static_cast<int64_t>(val.id) << ":" << static_cast<int64_t>(val.data) << " x"
		   << static_cast<int64_t>(val.count) << ")";
		return os;
	}

	void decrementCount(int8_t amount) {
		if (count - amount < 0) {
			count = 0;
		} else {
			count -= amount;
		}
		if (count <= 0) {
			id = ITEM_INVALID;
			data = 0;
		}
	}

	std::string str() const {
		std::ostringstream oss;
		oss << *this;
		return oss.str();
	}
};

namespace std {
template <>
struct hash<ItemStack> {
	size_t operator()(const ItemStack& s) const noexcept {
		size_t h = 0;
		hash_combine(h, s.id);
		hash_combine(h, s.count);
		hash_combine(h, s.data);
		return h;
	}
};
} // namespace std