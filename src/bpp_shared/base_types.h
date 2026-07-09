/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>

// zero-cost wrapper that gives integers a distinct type
template <typename UnderlyingT, typename TagT>
struct Branded {
	// names the wrapped integer for places that require a real type, like enum bases:
	// enum Items : ItemId::Underlying
	using Underlying = UnderlyingT;

	UnderlyingT m_value;

	constexpr Branded() = default;
	constexpr Branded(UnderlyingT value) : m_value(value) {}

	template <typename OtherUnderlyingT, typename OtherTagT>
	requires(!std::is_same_v<TagT, OtherTagT>) constexpr Branded(Branded<OtherUnderlyingT, OtherTagT>) = delete;

	constexpr operator UnderlyingT() const {
		return m_value;
	}

	template <typename OtherUnderlyingT, typename OtherTagT>
	requires(!std::is_same_v<TagT, OtherTagT>) constexpr bool operator==(Branded<OtherUnderlyingT, OtherTagT>) const =
	    delete;

	template <typename OtherUnderlyingT, typename OtherTagT>
	    requires(!std::is_same_v<TagT, OtherTagT>) constexpr auto operator<=>
	    (Branded<OtherUnderlyingT, OtherTagT>) const = delete;

	constexpr Branded& operator++() {
		++m_value;
		return *this;
	}

	constexpr Branded operator++(int) {
		Branded previous = *this;
		++m_value;
		return previous;
	}
};

template <typename UnderlyingT, typename TagT>
struct std::hash<Branded<UnderlyingT, TagT>> {
	size_t operator()(Branded<UnderlyingT, TagT> branded) const noexcept {
		return std::hash<UnderlyingT>{}(branded.m_value);
	}
};

// stuff that needs arithmetic like ticks and item amount/damage needs to stay unbranded
typedef int64_t TickTime;
typedef int8_t ItemAmount;
typedef int16_t ItemDamage;

using EntityId = Branded<int32_t, struct EntityIdTag>;
using ItemId = Branded<int16_t, struct ItemIdTag>;
using MapId = Branded<int16_t, struct MapIdTag>;
using WindowId = Branded<int8_t, struct WindowIdTag>;
using TransactionId = Branded<int16_t, struct TransactionIdTag>;

// protocol/nbt slots are numbered differently
using NetworkSlotId = Branded<int16_t, struct NetworkSlotIdTag>;
using NbtSlotId = Branded<int8_t, struct NbtSlotIdTag>;

static_assert(std::is_trivially_copyable_v<EntityId> && sizeof(EntityId) == sizeof(int32_t));
static_assert(std::is_trivially_copyable_v<ItemId> && sizeof(ItemId) == sizeof(int16_t));