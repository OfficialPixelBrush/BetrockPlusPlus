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
	ItemId m_id = Items::Id::INVALID;
	ItemAmount m_count = 0;
	ItemDamage m_data = 0; // This is "damage" in the og java but data makes more sense for what this is used for

	bool operator==(const ItemStack& o) const {
		return m_id == o.m_id && m_count == o.m_count && m_data == o.m_data;
	}
	bool operator!=(const ItemStack& o) const {
		return !(*this == o);
	}

	friend std::wostream& operator<<(std::wostream& wos, const ItemStack& val) {
		wos << "(" << static_cast<int64_t>(val.m_id) << ":" << static_cast<int64_t>(val.m_data) << " x"
		    << static_cast<int64_t>(val.m_count) << ")";
		return wos;
	}

	friend std::ostream& operator<<(std::ostream& os, const ItemStack& val) {
		os << "(" << static_cast<int64_t>(val.m_id) << ":" << static_cast<int64_t>(val.m_data) << " x"
		   << static_cast<int64_t>(val.m_count) << ")";
		return os;
	}

	void decrementCount(int8_t amount) {
		if (m_count - amount < 0) {
			m_count = 0;
		} else {
			m_count -= amount;
		}
		if (m_count <= 0) {
			m_id = Items::Id::INVALID;
			m_data = 0;
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
		hash_combine(h, s.m_id);
		hash_combine(h, s.m_count);
		hash_combine(h, s.m_data);
		return h;
	}
};
} // namespace std