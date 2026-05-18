/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#include "network_stream.h"
#include "buffer_stream.h"
#pragma once
class PacketStagingBuffer {
public:
    // Feed available socket bytes into the staging buffer.
    // Returns false if the socket was lost (errno / WSA error was fatal).
    bool feed(int socket);

    // True when a complete packet has been accumulated.
    bool ready() const { return totalSize > 0 && buf.size() >= totalSize; }

    // Move the accumulated bytes out as a BufferStream and reset for the
    // next packet.  Must only be called when ready() == true.
    BufferStream take();

    bool connectionLost() const { return lost; }

private:
    std::vector<uint8_t> buf;       // bytes accumulated so far
    size_t totalSize = 0;           // 0 = not yet known
    bool   lost      = false;

    // Read as many bytes as available from the socket (non-blocking) into buf,
    // up to `needed` bytes total.  Returns false on fatal error.
    bool drainSocket(int socket, size_t needed);

    // Given that buf already contains at least the size-header bytes, compute
    // and store totalSize.  Returns false if there are not yet enough bytes in
    // buf to make the determination (caller should drainSocket more first).
    bool computeTotalSize();

    // Scan the 0x7F-terminated entity metadata block starting at buf[startOffset].
    // Returns the total packet size on success, or SIZE_MAX if more bytes are needed.
    size_t scanMetadata(size_t startOffset);
};
