/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "blocks.h"
#include "numeric_structs.h"
class WorldAccess {
    public:
    WorldAccess();
    virtual ~WorldAccess() = default;
	// Convert a world-space position to a region-local chunk offset (-1..1, -1..1)
	virtual Int2 GetRegionChunkPos(const Int3 _wPos) const;
	virtual int FindTopSolidBlock(const int _wx, const int _wz);
	virtual int GetHeightValue(const int _wx, const int _wz);
	virtual double GetTemperatureAt(const int _wx, const int _wz);
	virtual double GetHumidityAt(const int _wx, const int _wz);
	virtual BlockType GetBlockId(const Int3 _wpos) const;
	virtual void SetBlock(const Int3 _wpos, const BlockType _type, const uint8_t _meta = 0,
	              const bool _keepTileEntity = false, const bool _updateNeighbors = true);
	virtual uint8_t GetSkyLight(const Int3 _wpos) const;
};