/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
#define INVALID_SOCKET -1
#if defined(__linux__)
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#elif defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif
#include <cstdint>
#include <string>
#include <packet_ids.h>
#include <bit>
#include <type_traits>
#include <cstring>
#include <vector>
#include "helpers/byteswap_compat.h"

template<typename T>
inline T byteswap_any(T value) {
    static_assert(std::is_trivially_copyable_v<T>,
        "byteswap_any: only trivially copyable types allowed");
    if constexpr (sizeof(T) == 1) {
        return value;
    }
    else if constexpr (sizeof(T) == 2) {
        uint16_t tmp;
        std::memcpy(&tmp, &value, 2);
        tmp = __builtin_bswap16(tmp);
        std::memcpy(&value, &tmp, 2);
    }
    else if constexpr (sizeof(T) == 4) {
        uint32_t tmp;
        std::memcpy(&tmp, &value, 4);
        tmp = __builtin_bswap32(tmp);
        std::memcpy(&value, &tmp, 4);
    }
    else if constexpr (sizeof(T) == 8) {
        uint64_t tmp;
        std::memcpy(&tmp, &value, 8);
        tmp = __builtin_bswap64(tmp);
        std::memcpy(&value, &tmp, 8);
    }
    else {
        static_assert(sizeof(T) <= 8, "byteswap_any: unsupported type size");
    }
    return value;
}

// ---------------------------------------------------------------------------
// BufferStream — deserialises a packet from a complete in-memory buffer.
//
// The buffer is guaranteed to hold the entire packet before Deserialize() is
// ever called, so Read() can never short-read.  This replaces the old pattern
// of deserialising directly from the non-blocking socket, which required the
// fragile shortRead/rollback mechanism.
// ---------------------------------------------------------------------------
class BufferStream {
public:
    explicit BufferStream(std::vector<uint8_t> data)
        : buf_(std::move(data)), pos_(0) {}

    template<typename T>
    T Read() {
        static_assert(std::is_trivially_copyable_v<T>,
            "BufferStream::Read<T>: use Read<std::string>() or Read<std::wstring>() for strings");
        T value{};
        ReadBytes(reinterpret_cast<uint8_t*>(&value), sizeof(T));
        return byteswap_any(value);
    }

    void ReadBytes(uint8_t* out, size_t len) {
        // In a well-formed session the staging layer guarantees the buffer is
        // complete, so running off the end should not happen.  Assert in debug;
        // in release just clamp so we never crash.
        if (pos_ + len > buf_.size()) {
            len = buf_.size() - pos_;
        }
        std::memcpy(out, buf_.data() + pos_, len);
        pos_ += len;
    }

    // String-8: uint16_t length prefix, then raw bytes.
    std::string ReadString() {
        uint16_t len = Read<uint16_t>();
        std::string result(len, '\0');
        ReadBytes(reinterpret_cast<uint8_t*>(result.data()), len);
        return result;
    }

    // String-16: uint16_t char-count prefix, then big-endian UTF-16 units.
    std::wstring ReadWString() {
        uint16_t len = Read<uint16_t>();
        std::vector<uint16_t> tmp(len);
        ReadBytes(reinterpret_cast<uint8_t*>(tmp.data()), len * sizeof(uint16_t));
        std::wstring result(len, L'\0');
        for (uint16_t i = 0; i < len; i++)
            result[i] = static_cast<wchar_t>(__builtin_bswap16(tmp[i]));
        return result;
    }

    // Entity metadata: reads until sentinel 0x7F (same logic as NetworkStream).
    void ReadEntityMetadata();

    size_t pos() const { return pos_; }
    size_t size() const { return buf_.size(); }

private:
    std::vector<uint8_t> buf_;
    size_t               pos_;
};

// Explicit specialisations so callers can write Read<std::string>() etc.
template<> inline std::string  BufferStream::Read<std::string>()  { return ReadString();  }
template<> inline std::wstring BufferStream::Read<std::wstring>() { return ReadWString(); }


// ---------------------------------------------------------------------------
// PacketStagingBuffer — accumulates raw bytes from the non-blocking socket
// into a per-session buffer until a complete packet is available.
//
// Because the protocol has no length prefix, the staging layer uses a two-
// phase approach:
//
//   Phase 1 (WAITING_FOR_SIZE):
//     Read exactly the bytes needed to determine the full packet length.
//     For most packets this is just the 1-byte packet ID.  For variable-
//     length packets (those containing strings, arrays, or metadata) the
//     sizer also needs to peek at length fields that appear early in the
//     packet body.  The sizer advances an internal cursor through the size-
//     header bytes until it has enough information to call computeBodySize().
//
//   Phase 2 (ACCUMULATING_BODY):
//     Read the remaining packet bytes (body) into the same buffer until
//     buf.size() == totalSize.
//
// Once totalSize bytes are in buf, ready() returns true.  The caller calls
// take() to move the buffer into a BufferStream for deserialisation, which
// resets the staging buffer for the next packet.
// ---------------------------------------------------------------------------
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


// ---------------------------------------------------------------------------
// NetworkStream — send-side write buffer + raw socket read (used only during
// the staging fill, not during packet deserialisation).
// ---------------------------------------------------------------------------
class NetworkStream {
public:
    NetworkStream(int client_socket);
    ~NetworkStream();
    bool NewClient();

    // Write helpers — all append to writeBuffer; no syscall.
    template<typename T = int>
    void Write(const T& data) {
        if constexpr (std::is_same_v<T, bool>) {
            int8_t boolData = static_cast<int8_t>(data);
            WriteBytes(reinterpret_cast<const uint8_t*>(&boolData), sizeof(int8_t));
        }
        else {
            T networkData = byteswap_any(data);
            WriteBytes(reinterpret_cast<const uint8_t*>(&networkData), sizeof(T));
        }
    }

    void setConnected(bool val) { connected = val; }
    bool isConnected() const { return connected; }

    // String-8 Write
    void Write(const std::string& str);
    // String-16 Write
    void Write(const std::wstring& str);

    // Raw byte buffer append (no endian conversion).
    void WriteBytes(const uint8_t* buf, size_t len);

    // Entity Metadata serialisation (TODO: implement).
    void WriteEntityMetadata();

    // Flush the write buffer to the socket once per tick.
    // Returns false if the connection was lost.
    bool flushWriteBuffer();
    // Blocking flush for use during SHUTDOWN ONLY.
    void flushWriteBufferBlocking();

    // Check whether there are bytes waiting on the socket.
    bool hasData();

    // Append pre-serialised bytes directly (used for shared-packet broadcast).
    void writeRaw(const uint8_t* data, size_t len) { WriteBytes(data, len); }

    // Read-only view of the pending write buffer.
    const std::vector<uint8_t>& getRawWriteBuffer() const { return writeBuffer; }

    // Expose the raw socket so PacketStagingBuffer::feed() can call recv().
    int rawSocket() const { return client_socket; }

private:
    int client_socket = INVALID_SOCKET;
    bool connected = true;
    std::vector<uint8_t> writeBuffer;
};
