/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
#include "network_stream.h"
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