/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once

#include "../numeric_structs.h"
#include <cstdint>
#include <vector>

namespace Map {
enum class Icon : uint8_t {
	WhiteArrow,
	GreenArrow,
	RedArrow,
	BlueArrow,
	WhiteCross
};

struct MarkerPlacement {
	Icon icon;
	uint8_t rotation;
	Byte2 position;
};
MarkerPlacement MakeMarker(const Icon _icon, const float _rotation, const Byte2 _position);
void AppendPixel(std::vector<uint8_t>& _mapData, const uint8_t _value);
void InitGraphics(std::vector<uint8_t>& _mapData, const Byte2 _offset);
void PlaceMarkers(std::vector<uint8_t>& _mapData, const std::vector<MarkerPlacement> _markers);
}; // namespace Map