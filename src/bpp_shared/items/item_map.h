/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include <cstdint>
#include <vector>
#include "../numeric_structs.h"
#include "../logger/logger.h"
#include "../enums/network/packet_data.h"

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

MarkerPlacement MakeMarker(const Icon _icon, const float _rotation, const Byte2 _position) {
    uint8_t rot = static_cast<uint8_t>(std::round((std::fmod(_rotation, 360.0f) / 360.0f) * 16.0f));
    return MarkerPlacement {
        .icon = _icon,
        .rotation = rot,
        .position = _position
    };
}

void AppendPixel(std::vector<uint8_t>& _mapData, const uint8_t _value) {
    if (_mapData.size() > INT8_MAX) {
        GlobalLogger().warn << "Attempted to write too much map data in a single packet! Rejected! (" << _mapData.size() << "/" << INT8_MAX << ")\n";
        return;
    }
    _mapData.push_back(_value);
}

void InitGraphics(std::vector<uint8_t>& _mapData, const Byte2 _offset) {
    _mapData = { PacketData::MapDataType::GRAPHICS, static_cast<uint8_t>(_offset.x) , static_cast<uint8_t>(_offset.z) };
}

void PlaceMarkers(std::vector<uint8_t>& _mapData, const std::vector<MarkerPlacement> _markers) {
    _mapData.clear();
    _mapData.reserve(1 + (_markers.size() * 3));
    _mapData.push_back(PacketData::MapDataType::ICON);
    for (const auto& m : _markers) {
        _mapData.push_back(
            static_cast<uint8_t>((m.rotation << 4) | static_cast<uint8_t>(m.icon))
        );
        _mapData.push_back(m.position.x);
        _mapData.push_back(m.position.z);
    }
}
};