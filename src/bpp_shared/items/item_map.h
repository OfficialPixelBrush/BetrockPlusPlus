/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include <cstdint>
#include <vector>
#include "../numeric_structs.h"

enum class MapIcon : uint8_t {
    WhiteArrow,
    GreenArrow,
    RedArrow,
    BlueArrow,
    WhiteCross
};

struct MarkerPlacement {
    MapIcon icon;
    uint8_t rotation;
    Byte2 position;
};

MarkerPlacement MakeMarker(MapIcon _icon, float _rotation, Byte2 _position) {
    uint8_t rot = static_cast<uint8_t>(std::round((std::fmod(_rotation, 360.0f) / 360.0f) * 16.0f));
    return MarkerPlacement {
        .icon = _icon,
        .rotation = rot,
        .position = _position
    };
}

void AppendPixel(std::vector<uint8_t>& _mapData, uint8_t _value) {
    if (_mapData.size() > INT8_MAX) {
        GlobalLogger().warn << "Attempted to write too much map data in a single packet! Rejected!"
        return;
    }
    _mapData.push_back(value);
}

void InitGraphics(std::vector<uint8_t>& _mapData, Byte2 _offset) {
    _mapData = { PacketData::MapDataType::GRAPHICS, _offset.x , _offset.z };
}

void PlaceMarker(std::vector<uint8_t>& _mapData, std::vector<MarkerPlacement> _markers) {
    _mapData.clear();
    _mapData.reserve(1 + _markers.size() * sizeof(MarkerPlacement));
    _mapData.push_back(PacketData::MapDataType::ICON);
    for (auto& m : _markers) {
        _mapData.push_back(m.rotation << 4 | static_cast<uint8_t>(m.icon));
        _mapData.push_back(m.position.x);
        _mapData.push_back(m.position.z);
    }
}