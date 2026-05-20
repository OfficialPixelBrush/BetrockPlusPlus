/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#include "packet_staging_buffer.h"

// How many bytes we need in buf before we can determine totalSize
// for each packet ID.  0 means "only the ID byte is needed".
// SIZE_NEEDS_BODY means we need to peek at variable-length fields
// and the computeTotalSize() function handles it explicitly.
static constexpr size_t SIZE_NEEDS_BODY = SIZE_MAX;

// Returns the minimum number of bytes (including the 1-byte packet ID)
// we need in buf before computeTotalSize() can do its job.
static size_t sizeHeaderLength(PacketId id) {
    switch (id) {
    // ---- fixed-size packets: 1 (id) is all we need ----
    case PacketId::KeepAlive:               return 1;
    case PacketId::SetTime:                 return 1;
    case PacketId::SetEquipment:            return 1;
    case PacketId::SetSpawnPosition:        return 1;
    case PacketId::InteractWithEntity:      return 1;
    case PacketId::SetHealth:               return 1;
    case PacketId::Respawn:                 return 1;
    case PacketId::PlayerMovement:          return 1;
    case PacketId::PlayerPosition:          return 1;
    case PacketId::PlayerRotation:          return 1;
    case PacketId::PlayerPositionAndRotation: return 1;
    case PacketId::MineBlock:               return 1;
    case PacketId::SetHotbarSlot:           return 1;
    case PacketId::InteractWithBlock:       return 1;
    case PacketId::Animation:               return 1;
    case PacketId::PlayerAction:            return 1;
    case PacketId::PlayerInput:             return 1;
    case PacketId::EntityVelocity:          return 1;
    case PacketId::DespawnEntity:           return 1;
    case PacketId::EntityMovement:          return 1;
    case PacketId::EntityPosition:          return 1;
    case PacketId::EntityRotation:          return 1;
    case PacketId::EntityPositionAndRotation: return 1;
    case PacketId::TeleportEntity:          return 1;
    case PacketId::EntityEvent:             return 1;
    case PacketId::AddPassenger:            return 1;
    case PacketId::SetChunkVisibility:      return 1;
    case PacketId::SetBlock:                return 1;
    case PacketId::BlockEvent:              return 1;
    case PacketId::WorldEvent:              return 1;
    case PacketId::GameEvent:               return 1;
    case PacketId::LightningBolt:           return 1;
    case PacketId::CloseContainer:          return 1;
    case PacketId::ContainerData:           return 1;
    case PacketId::ContainerTransaction:    return 1;
    case PacketId::IncrementStatistic:      return 1;
    case PacketId::CollectItem:             return 1;

    // ---- PlaceBlock: need id(1)+x(4)+y(1)+z(4)+face(1)+item.id(2) = 13 bytes ----
    case PacketId::PlaceBlock:              return 13;

    // ---- ClickSlot: need id(1)+windowId(1)+slotId(2)+rightClick(1)+transactionId(2)+shift(1)+item.id(2) = 10 bytes ----
    case PacketId::ClickSlot:               return 10;

    // ---- SetSlot: id(1)+windowId(1)+slotId(2)+item.id(2) = 6 bytes ----
    case PacketId::SetSlot:                 return 6;

    // ---- SpawnObject: id(1)+entityId(4)+objectType(1)+x(4)+y(4)+z(4)+owner(4) = 22 bytes ----
    case PacketId::SpawnObject:             return 22;

    // ---- Variable-length: need to peek at length fields in the body ----
    case PacketId::Login:                   return SIZE_NEEDS_BODY;
    case PacketId::PreLogin:                return SIZE_NEEDS_BODY;
    case PacketId::ChatMessage:             return SIZE_NEEDS_BODY;
    case PacketId::SpawnPlayer:             return SIZE_NEEDS_BODY;
    case PacketId::SpawnPainting:           return SIZE_NEEDS_BODY;
    case PacketId::SpawnMob:               return SIZE_NEEDS_BODY;
    case PacketId::EntityMetadata:         return SIZE_NEEDS_BODY;
    case PacketId::OpenContainer:          return SIZE_NEEDS_BODY;
    case PacketId::FillContainer:          return SIZE_NEEDS_BODY;
    case PacketId::UpdateSign:             return SIZE_NEEDS_BODY;
    case PacketId::ItemData:               return SIZE_NEEDS_BODY;
    case PacketId::Disconnect:             return SIZE_NEEDS_BODY;
    case PacketId::Chunk:                  return SIZE_NEEDS_BODY;
    case PacketId::SetMultipleBlocks:      return SIZE_NEEDS_BODY;
    case PacketId::Explosion:              return SIZE_NEEDS_BODY;

    default:
        return SIZE_NEEDS_BODY; // unknown — fall through to the body reader
    }
}

// Compute the fixed body size (NOT counting the 1-byte packet ID) for packets
// where sizeHeaderLength() returned 1, i.e. the size is completely determined
// by the packet ID alone.
static size_t fixedBodySize(PacketId id) {
    switch (id) {
    case PacketId::KeepAlive:               return 0;
    case PacketId::SetTime:                 return 8;
    case PacketId::SetEquipment:            return 4+2+2+2; // entityId+slot+itemId+meta
    case PacketId::SetSpawnPosition:        return 4+4+4;
    case PacketId::InteractWithEntity:      return 4+4+1;
    case PacketId::SetHealth:               return 2;
    case PacketId::Respawn:                 return 1;  // Dimension is int8
    case PacketId::PlayerMovement:          return 1;
    case PacketId::PlayerPosition:          return 8+8+8+8+1; // x+y+cameraY+z+onGround
    case PacketId::PlayerRotation:          return 4+4+1;
    case PacketId::PlayerPositionAndRotation: return 8+8+8+8+4+4+1;
    case PacketId::MineBlock:               return 1+4+1+4+1; // status+x+y+z+face
    case PacketId::SetHotbarSlot:           return 2;
    case PacketId::InteractWithBlock:       return 4+1+4+1+4; // entityId+interactionId+x+y+z
    case PacketId::Animation:               return 4+1;
    case PacketId::PlayerAction:            return 4+1;
    case PacketId::PlayerInput:             return 4+4+4+4+1+1;
    case PacketId::EntityVelocity:          return 4+2+2+2;
    case PacketId::DespawnEntity:           return 4;
    case PacketId::EntityMovement:          return 0;
    case PacketId::EntityPosition:          return 4+1+1+1;
    case PacketId::EntityRotation:          return 4+1+1;
    case PacketId::EntityPositionAndRotation: return 4+1+1+1+1+1;
    case PacketId::TeleportEntity:          return 4+4+4+4+1+1;
    case PacketId::EntityEvent:             return 4+1;
    case PacketId::AddPassenger:            return 4+4;
    case PacketId::SetChunkVisibility:      return 4+4+1;
    case PacketId::SetBlock:                return 4+1+4+1+1;
    case PacketId::BlockEvent:              return 4+1+4+1+1;
    case PacketId::WorldEvent:              return 4+4+1+4+4; // x+z+y+eventId+data
    case PacketId::GameEvent:               return 1;
    case PacketId::LightningBolt:           return 4+1+4+4+4;
    case PacketId::CloseContainer:          return 1;
    case PacketId::ContainerData:           return 1+2+2;
    case PacketId::ContainerTransaction:    return 1+2+1;
    case PacketId::IncrementStatistic:      return 4+1;
    case PacketId::CollectItem:             return 4+4;
    default:                                return 0;
    }
}

// Helper: big-endian uint16 from two bytes in buf at offset pos (no bounds check –
// caller ensures buf is large enough).
static uint16_t peekBE16(const std::vector<uint8_t>& buf, size_t pos) {
    return static_cast<uint16_t>((uint16_t(buf[pos]) << 8) | buf[pos + 1]);
}
static int16_t peekBE16s(const std::vector<uint8_t>& buf, size_t pos) {
    return static_cast<int16_t>(peekBE16(buf, pos));
}
static uint32_t peekBE32(const std::vector<uint8_t>& buf, size_t pos) {
    return (uint32_t(buf[pos]) << 24) | (uint32_t(buf[pos+1]) << 16) |
           (uint32_t(buf[pos+2]) << 8) | buf[pos+3];
}

// Given that buf has already been filled up to sizeHeaderLength(id) bytes,
// compute totalSize.  Returns false if we still need more bytes (variable-
// length packets where the size header itself contains lengths that haven't
// arrived yet).
bool PacketStagingBuffer::computeTotalSize() {
    if (buf.empty()) return false;

    PacketId id = static_cast<PacketId>(buf[0]);
    size_t hdr  = sizeHeaderLength(id);

    if (hdr != SIZE_NEEDS_BODY) {
        // ---- Semi-fixed: we have enough to decide ----
        if (buf.size() < hdr) return false; // need more header bytes

        size_t body = fixedBodySize(id);

        switch (id) {
        case PacketId::PlaceBlock: {
            // Wire (after packet id byte 0): x(4)+y(1)+z(4)+face(1) = 10 bytes, item.id(2) at [11..12]
            if (buf.size() < 13) return false;
            int16_t itemId = peekBE16s(buf, 11);
            totalSize = 13 + (itemId >= 0 ? 3 : 0); // +count(1)+data(2)
            return true;
        }
        case PacketId::ClickSlot: {
            // Wire: id(1)+windowId(1)+slotId(2)+rightClick(1)+transactionId(2)+shift(1)+item.id(2)
            // offset: 0   1          2          4              5               7       8
            // need 10 bytes to have read item.id
            if (buf.size() < 10) return false;
            int16_t itemId = peekBE16s(buf, 8);
            totalSize = 10 + (itemId != -1 ? 3 : 0);
            return true;
        }
        case PacketId::SetSlot: {
            // Wire: id(1)+windowId(1)+slotId(2)+item.id(2)
            // offset: 0   1          2          4
            if (buf.size() < 6) return false;
            int16_t itemId = peekBE16s(buf, 4);
            totalSize = 6 + (itemId != -1 ? 3 : 0);
            return true;
        }
        case PacketId::SpawnObject: {
            // Wire: id(1)+entityId(4)+objectType(1)+x(4)+y(4)+z(4)+owner(4)
            // offset:0    1           5             6    10   14   18
            // need 22 bytes to have read all of owner
            if (buf.size() < 22) return false;
            uint32_t owner = peekBE32(buf, 18);
            totalSize = 22 + (owner != 0 ? 6 : 0); // +3*int16 velocity
            return true;
        }
        default:
            // Truly fixed-size packet
            totalSize = 1 + body;
            return true;
        }
    }

    // ---- SIZE_NEEDS_BODY: variable-length — parse length fields on demand ----
    // We progressively read length-determining fields as they arrive.
    // Each case returns false if it needs more bytes than are currently in buf.

    // Utility: reads a big-endian uint16 string length from buf[pos] and returns
    // the number of bytes that string occupies on the wire (2 + len).
    // Returns SIZE_MAX if pos+2 > buf.size() (not enough bytes yet).
    auto wstrLen = [&](size_t pos) -> size_t {
        if (pos + 2 > buf.size()) return SIZE_MAX;
        return 2 + size_t(peekBE16(buf, pos)) * 2; // 2-byte prefix + len*2 UTF-16 bytes
    };
    auto strLen = [&](size_t pos) -> size_t {
        if (pos + 2 > buf.size()) return SIZE_MAX;
        return 2 + size_t(peekBE16(buf, pos)); // 2-byte prefix + len bytes
    };

    switch (id) {
    // ---- Login: id(1)+entityId(4)+username(wstr)+worldSeed(8)+dimension(1) ----
    case PacketId::Login: {
        size_t usLen = wstrLen(5);
        if (usLen == SIZE_MAX) return false;
        totalSize = 1 + 4 + usLen + 8 + 1;
        return true;
    }
    // ---- PreLogin: id(1)+username_or_hash(wstr) ----
    case PacketId::PreLogin: {
        size_t usLen = wstrLen(1);
        if (usLen == SIZE_MAX) return false;
        totalSize = 1 + usLen;
        return true;
    }
    // ---- ChatMessage: id(1)+message(wstr) ----
    case PacketId::ChatMessage: {
        size_t msgLen = wstrLen(1);
        if (msgLen == SIZE_MAX) return false;
        totalSize = 1 + msgLen;
        return true;
    }
    // ---- Disconnect: id(1)+reason(wstr) ----
    case PacketId::Disconnect: {
        size_t rLen = wstrLen(1);
        if (rLen == SIZE_MAX) return false;
        totalSize = 1 + rLen;
        return true;
    }
    // ---- SpawnPlayer: id(1)+entityId(4)+username(wstr)+x(4)+y(4)+z(4)+yaw(1)+pitch(1)+heldItem(2) ----
    case PacketId::SpawnPlayer: {
        size_t usLen = wstrLen(5);
        if (usLen == SIZE_MAX) return false;
        totalSize = 1 + 4 + usLen + 4 + 4 + 4 + 1 + 1 + 2;
        return true;
    }
    // ---- SpawnPainting: id(1)+entityId(4)+title(wstr)+x(4)+y(4)+z(4)+direction(4) ----
    case PacketId::SpawnPainting: {
        size_t titLen = wstrLen(5);
        if (titLen == SIZE_MAX) return false;
        totalSize = 1 + 4 + titLen + 4 + 4 + 4 + 4;
        return true;
    }
    // ---- OpenContainer: id(1)+windowId(1)+windowType(1)+title(str8)+slotCount(1) ----
    case PacketId::OpenContainer: {
        size_t titLen = strLen(3);
        if (titLen == SIZE_MAX) return false;
        totalSize = 1 + 1 + 1 + titLen + 1;
        return true;
    }
    // ---- UpdateSign: id(1)+x(4)+y(2)+z(4)+line0..3(wstr each) ----
    case PacketId::UpdateSign: {
        size_t off = 11; // after id+x+y+z
        for (int i = 0; i < 4; i++) {
            size_t lLen = wstrLen(off);
            if (lLen == SIZE_MAX) return false;
            off += lLen;
        }
        totalSize = off;
        return true;
    }
    // ---- ItemData: id(1)+itemId(2)+mapId(2)+dataSize(1)+data[dataSize] ----
    case PacketId::ItemData: {
        if (buf.size() < 6) return false;
        uint8_t dataSize = buf[5];
        totalSize = 1 + 2 + 2 + 1 + dataSize;
        return true;
    }
    // ---- Chunk: id(1)+chunkX(4)+chunkY(2)+chunkZ(4)+sX(1)+sY(1)+sZ(1)+compressedSize(4)+data ----
    case PacketId::Chunk: {
        if (buf.size() < 18) return false;
        uint32_t compSize = peekBE32(buf, 14);
        totalSize = 18 + compSize;
        return true;
    }
    // ---- SetMultipleBlocks: id(1)+chunkX(4)+chunkZ(4)+count(2)+coords(count*2)+types(count*1)+meta(count*1) ----
    case PacketId::SetMultipleBlocks: {
        if (buf.size() < 11) return false;
        uint16_t count = peekBE16(buf, 9);
        totalSize = 11 + size_t(count) * (2 + 1 + 1);
        return true;
    }
    // ---- FillContainer: id(1)+windowId(1)+count(2) then for each: item.id(2) [+count(1)+data(2)] ----
    case PacketId::FillContainer: {
        if (buf.size() < 4) return false;
        uint16_t numSlots = peekBE16(buf, 2);
        // We need to walk every slot to handle the conditional item fields.
        size_t off = 4;
        for (uint16_t i = 0; i < numSlots; i++) {
            if (buf.size() < off + 2) return false; // need item.id
            int16_t itemId = peekBE16s(buf, off);
            off += 2;
            if (itemId != -1) {
                off += 3; // count(1) + data(2)
            }
        }
        totalSize = off;
        return true;
    }
    // ---- Explosion: id(1)+x(8)+y(8)+z(8)+radius(4)+numBlocks(4)+blocks(numBlocks) ----
    case PacketId::Explosion: {
        if (buf.size() < 33) return false; // id+xyz+radius+numBlocks = 33 bytes
        uint32_t numBlocks = peekBE32(buf, 29);
        totalSize = 33 + numBlocks;
        return true;
    }

    // ---- SpawnMob / EntityMetadata: variable metadata at the end ----
    // These need to scan the 0x7F-terminated metadata inline.
    case PacketId::SpawnMob: {
        // Fixed prefix: id(1)+entityId(4)+mobType(1)+x(4)+y(4)+z(4)+yaw(1)+pitch(1) = 20
        size_t off = 20;
        size_t r = scanMetadata(off);
        if (r == SIZE_MAX) return false;
        totalSize = r;
        return true;
    }
    case PacketId::EntityMetadata: {
        // Fixed prefix: id(1)+entityId(4) = 5
        size_t off = 5;
        size_t r = scanMetadata(off);
        if (r == SIZE_MAX) return false;
        totalSize = r;
        return true;
    }

    default:
        // Unknown packet — we can't determine the size.
        // Mark as needing 1 byte more than we have so we keep accumulating;
        // in practice the caller will disconnect on an unhandled packet.
        totalSize = 0;
        return false;
    }
}

// Scan the metadata block starting at buf[startOffset].
// Returns the total packet size (including bytes before startOffset) on success,
// or SIZE_MAX if more bytes are needed.
size_t PacketStagingBuffer::scanMetadata(size_t startOffset) {
    size_t off = startOffset;
    while (true) {
        if (off >= buf.size()) return SIZE_MAX;
        uint8_t val = buf[off++];
        if (val == 0x7F) return off; // done — off is the total packet size
        PacketData::EntityMetadata::Type type =
            PacketData::EntityMetadata::Type(val >> 5);
        switch (type) {
        case PacketData::EntityMetadata::Type::BYTE:
            off += 1; break;
        case PacketData::EntityMetadata::Type::SHORT:
            off += 2; break;
        case PacketData::EntityMetadata::Type::INTEGER:
        case PacketData::EntityMetadata::Type::FLOAT:
            off += 4; break;
        case PacketData::EntityMetadata::Type::STRING: {
            if (off + 2 > buf.size()) return SIZE_MAX;
            uint16_t slen = peekBE16(buf, off);
            off += 2 + slen;
            break;
        }
        case PacketData::EntityMetadata::Type::ITEM: {
            if (off + 2 > buf.size()) return SIZE_MAX;
            int16_t itemId = peekBE16s(buf, off);
            off += 2;
            if (itemId != -1) off += 3; // count(1)+data(2)
            break;
        }
        case PacketData::EntityMetadata::Type::COORINDATES:
            off += 12; break;
        default:
            return SIZE_MAX; // corrupt metadata
        }
        if (off > buf.size()) return SIZE_MAX;
    }
}

bool PacketStagingBuffer::drainSocket(int socket, size_t needed) {
    while (buf.size() < needed) {
        size_t toRead = needed - buf.size();
        size_t oldSize = buf.size();
        buf.resize(oldSize + toRead);
#if defined(_WIN32) || defined(_WIN64)
        int result = recv(socket,
            reinterpret_cast<char*>(buf.data() + oldSize),
            static_cast<int>(toRead), 0);
        if (result <= 0) {
            buf.resize(oldSize);
            if (result == 0) { lost = true; return false; }
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK || err == WSAETIMEDOUT) return false;
            lost = true;
            return false;
        }
        buf.resize(oldSize + static_cast<size_t>(result));
#else
        ssize_t result = recv(socket,
            buf.data() + oldSize, toRead, 0);
        if (result <= 0) {
            buf.resize(oldSize);
            if (result == 0) { lost = true; return false; }
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ETIMEDOUT)
                return false;
            lost = true;
            return false;
        }
        buf.resize(oldSize + static_cast<size_t>(result));
#endif
    }
    return true;
}

bool PacketStagingBuffer::feed(int socket) {
    if (lost) return false;

    // Opportunistically drain everything the OS has buffered
    u_long available = 0;
    #if defined(_WIN32) || defined(_WIN64)
        ioctlsocket(socket, FIONREAD, &available);
    #else
        ioctl(socket, FIONREAD, &available);
    #endif
    if (available > 0)
        drainSocket(socket, buf.size() + available);

    // Now try to determine size / check completeness
    if (totalSize == 0) computeTotalSize();
    if (totalSize > 0 && buf.size() < totalSize)
        drainSocket(socket, totalSize);  // one more attempt for the remainder

    return !lost;
}

BufferStream PacketStagingBuffer::take() {
    BufferStream bs(std::move(buf));
    buf.clear();
    totalSize = 0;
    return bs;
}