/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#pragma once
#include "NBT.h"
#include <bit>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <unordered_map>

enum TagType : uint8_t {
    TAG_END, TAG_BYTE, TAG_SHORT, TAG_INT, TAG_LONG,
    TAG_FLOAT, TAG_DOUBLE, TAG_BYTEARRAY,
    TAG_STRING, TAG_LIST, TAG_COMPOUND, TAG_INTARRAY
};

struct Tag {
    TagType type = TAG_END;
    std::string name;

    // Leaf values
    int8_t   byteValue = 0;
    int16_t  shortValue = 0;
    int32_t  intValue = 0;
    int64_t  longValue = 0;
    float    floatValue = 0.0f;
    double   doubleValue = 0.0;
    std::vector<int8_t> byteArray;
    std::vector<int32_t> intArray;
    std::string stringValue;

    // Container values
    TagType listType = TAG_END;  // element type for TAG_LIST
    std::vector<Tag> list;
    std::unordered_map<std::string, Tag> compound;

    // Typed getters; throw if wrong type
    int8_t   getByte()   const { expect(TAG_BYTE);      return byteValue; }
    int16_t  getShort()  const { expect(TAG_SHORT);     return shortValue; }
    int32_t  getInt()    const { expect(TAG_INT);       return intValue; }
    int64_t  getLong()   const { expect(TAG_LONG);      return longValue; }
    float    getFloat()  const { expect(TAG_FLOAT);     return floatValue; }
    double   getDouble() const { expect(TAG_DOUBLE);    return doubleValue; }
    const std::vector<int8_t>& getByteArray() const { expect(TAG_BYTEARRAY); return byteArray; }
    const std::vector<int32_t>& getIntArray() const { expect(TAG_INTARRAY); return intArray; }
    const std::string& getString()            const { expect(TAG_STRING);    return stringValue; }
    const std::vector<Tag>& getList()         const { expect(TAG_LIST);      return list; }
    const std::unordered_map<std::string, Tag>& getCompound()  const { expect(TAG_COMPOUND);  return compound; }

    // Compound lookup helpers
    bool has(const std::string& key) const {
        return compound.count(key) > 0;
    }

    const Tag& get(const std::string& key) const {
        auto it = compound.find(key);
        if (it == compound.end())
            throw std::runtime_error("NBT key not found: " + key);
        return it->second;
    }

private:
    void expect(TagType t) const {
        if (type != t)
            throw std::runtime_error("NBT type mismatch");
    }
};

struct NBTwriter {
    NBTwriter() = default;
    NBTwriter(std::vector<uint8_t>& out, Tag& root) {
        // root should be a TAG_Compound with whatever name you want (usually "")
        // writeTag handles type byte + name + payload + TAG_END automatically
        writeTag(out, root);
    }

    int64_t writeTag(std::vector<uint8_t>& out, const Tag& tag, bool payload = false) {
        if (!payload) out.push_back(uint8_t(tag.type));
        if (!payload && tag.type != TAG_END) writeString(out, tag.name);

        switch (tag.type) {
        case TAG_END: break;
        case TAG_BYTE:   out.push_back(uint8_t(tag.byteValue)); break;
        case TAG_SHORT:  writeI16(out, tag.shortValue); break;
        case TAG_INT:    writeI32(out, tag.intValue); break;
        case TAG_LONG:   writeI64(out, tag.longValue); break;
        case TAG_FLOAT:  writeF32(out, tag.floatValue); break;
        case TAG_DOUBLE: writeF64(out, tag.doubleValue); break;
        case TAG_STRING: writeString(out, tag.stringValue); break;

        case TAG_BYTEARRAY: {
            writeI32(out, int32_t(tag.byteArray.size()));
            for (int8_t b : tag.byteArray)
                out.push_back(uint8_t(b));
            break;
        }

        case TAG_INTARRAY: {
            writeI32(out, int32_t(tag.intArray.size()));
            for (int32_t b : tag.intArray)
                writeI32(out, b);
            break;
        }

        case TAG_LIST: {
            writeI8(out, int8_t(tag.listType));
            writeI32(out, int32_t(tag.list.size()));
            for (const Tag& element : tag.list)
                writeTag(out, element, true);
            break;
        }

        case TAG_COMPOUND: {
            for (const auto& [key, child] : tag.compound)
                writeTag(out, child);
            // TAG_END terminates the compound
            out.push_back(uint8_t(TAG_END));
            break;
        }

        default:
            throw std::runtime_error("Unknown tag type: " + std::to_string(tag.type));
        }

        return 0;
    }

    // Write helpers
    void writeI32(std::vector<uint8_t>& out, int32_t v) {
        uint32_t u = uint32_t(v);
        out.push_back((u >> 24) & 0xFF);
        out.push_back((u >> 16) & 0xFF);
        out.push_back((u >> 8) & 0xFF);
        out.push_back(u & 0xFF);
    }

    void writeI64(std::vector<uint8_t>& out, int64_t v) {
        writeI32(out, int32_t((uint64_t(v) >> 32) & 0xFFFFFFFF));
        writeI32(out, int32_t((uint64_t(v) & 0xFFFFFFFF)));
    }

    void writeI16(std::vector<uint8_t>& out, int16_t v) {
        uint16_t u = uint16_t(v);
        out.push_back((u >> 8) & 0xFF);
        out.push_back(u & 0xFF);
    }

    void writeI8(std::vector<uint8_t>& out, int8_t v) {
        out.push_back(uint8_t(v));
    }

    void writeF32(std::vector<uint8_t>& out, float v) {
        uint32_t raw;
        memcpy(&raw, &v, 4);
        writeI32(out, int32_t(raw));
    }

    void writeF64(std::vector<uint8_t>& out, double v) {
        uint64_t raw;
        memcpy(&raw, &v, 8);
        writeI64(out, int64_t(raw));
    }

    void writeString(std::vector<uint8_t>& out, const std::string& s) {
        writeI16(out, int16_t(s.size()));
        out.insert(out.end(), s.begin(), s.end());
    }
};

struct NBTParser {
    uint8_t* data;
    int64_t length;
    int64_t pos;
    Tag root;

    NBTParser() = default;
    NBTParser(uint8_t* data, int64_t length) : data(data), length(length), pos(0) {
        root = parseTag();
        if (root.type != TAG_COMPOUND)
            throw std::runtime_error("NBT root tag is not a compound!");
    }

    // Parse a tag, either with type and name bytes (parseTag) or just a payload (parsePayload)
    Tag parsePayload(TagType type, const std::string& name = "") {
        Tag tag{ type, name };

        switch (type) {
        case TAG_BYTE:   tag.byteValue = readI8(); break;
        case TAG_SHORT:  tag.shortValue = readI16(); break;
        case TAG_INT:    tag.intValue = readI32(); break;
        case TAG_LONG:   tag.longValue = readI64(); break;
        case TAG_FLOAT:  tag.floatValue = readF32(); break;
        case TAG_DOUBLE: tag.doubleValue = readF64(); break;
        case TAG_STRING: tag.stringValue = readString(); break;

        case TAG_BYTEARRAY: {
            int32_t count = readI32();
            tag.byteArray.reserve(size_t(count));
            for (int i = 0; i < count; i++) tag.byteArray.push_back(readI8());
            break;
        }

        case TAG_INTARRAY: {
            int32_t count = readI32();
            tag.intArray.reserve(size_t(count));
            for (int i = 0; i < count; i++) tag.intArray.push_back(readI32());
            break;
        }

        case TAG_LIST: {
            int8_t innerType = readI8();
            int32_t count = readI32();

            if (innerType == TAG_END && count > 0)
                throw std::runtime_error("Invalid TAG_List");

            tag.list.reserve(size_t(count));
            for (int i = 0; i < count; i++)
                tag.list.push_back(parsePayload(TagType(innerType)));

            tag.listType = TagType(innerType);
            break;
        }

        case TAG_COMPOUND: {
            while (true) {
                Tag child = parseTag();
                if (child.type == TAG_END) break;
                tag.compound[child.name] = std::move(child);
            }
            break;
        }

        default:
            throw std::runtime_error("Unsupported payload type in list");
        }

        return tag;
    }

    // Parse a tag including its type byte and name
    Tag parseTag() {
        if (pos >= length) throw std::runtime_error("Unexpected end of NBT data");

        TagType type = TagType(data[pos++]);
        if (type == TAG_END)
            return Tag{ TAG_END, "" };   // no name for TAG_End

        std::string name = readString();
        return parsePayload(type, name);
    }

    // Read helpers
    int32_t readI32() {
        if (pos + 4 > length) throw std::runtime_error("NBT: unexpected end");
        uint32_t v = (uint32_t(data[pos]) << 24)
            | (uint32_t(data[pos + 1]) << 16)
            | (uint32_t(data[pos + 2]) << 8)
            | (uint32_t(data[pos + 3]));
        pos += 4;
        return int32_t(v);
    }

    int64_t readI64() {
        uint32_t hi = uint32_t(readI32());
        uint32_t lo = uint32_t(readI32());
        return int64_t((uint64_t(hi) << 32) | lo);
    }

    int16_t readI16() {
        if (pos + 2 > length) throw std::runtime_error("NBT: i16 out of bounds");
        uint16_t v = static_cast<uint16_t>(
            (static_cast<uint16_t>(data[pos]) << 8)
            | static_cast<uint16_t>(data[pos + 1])
        );
        pos += 2;
        return int16_t(v);
    }

    int8_t readI8() {
        if (pos >= length) throw std::runtime_error("NBT: i8 out of bounds");
        return int8_t(data[pos++]);
    }

    float readF32() {
        uint32_t raw = uint32_t(readI32());
        float f;
        memcpy(&f, &raw, 4);
        return f;
    }

    double readF64() {
        uint64_t raw = uint64_t(readI64());
        double d;
        memcpy(&d, &raw, 8);
        return d;
    }

    std::string readString() {
        uint16_t len = uint16_t(readI16());
        if (pos + len > length) throw std::runtime_error("NBT: string out of bounds");
        std::string s(reinterpret_cast<const char*>(data) + pos, len);
        pos += len;
        return s;
    }
};