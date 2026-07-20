/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "nbt.h"
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

enum TagType : uint8_t {
	TAG_END,
	TAG_BYTE,
	TAG_SHORT,
	TAG_INT,
	TAG_LONG,
	TAG_FLOAT,
	TAG_DOUBLE,
	TAG_BYTEARRAY,
	TAG_STRING,
	TAG_LIST,
	TAG_COMPOUND,
	TAG_INTARRAY
};

struct Tag {
	TagType type = TAG_END;
	std::string name;

	// Leaf values
	// do this as an anonymous union, since they can share memory
	union {
		int8_t byteValue;
		int16_t shortValue;
		int32_t intValue;
		// this is enough to init all of these
		int64_t longValue = 0;
		float floatValue;
		// all bits as 0 is also 0 in double!
		double doubleValue;
	};
	std::vector<int8_t> byteArray = {};
	std::vector<int32_t> intArray = {};
	std::string stringValue = "";

	// Container values
	TagType listType = TAG_END; // element type for TAG_LIST
	std::vector<Tag> list = {};
	std::unordered_map<std::string, Tag> compound = {};

	// Typed getters; throw if wrong type
	int8_t GetByte() const {
		Expect(TAG_BYTE);
		return byteValue;
	}
	int16_t GetShort() const {
		Expect(TAG_SHORT);
		return shortValue;
	}
	int32_t GetInt() const {
		Expect(TAG_INT);
		return intValue;
	}
	int64_t GetLong() const {
		Expect(TAG_LONG);
		return longValue;
	}
	float GetFloat() const {
		Expect(TAG_FLOAT);
		return floatValue;
	}
	double GetDouble() const {
		Expect(TAG_DOUBLE);
		return doubleValue;
	}
	const std::vector<int8_t>& GetByteArray() const {
		Expect(TAG_BYTEARRAY);
		return byteArray;
	}
	const std::vector<int32_t>& GetIntArray() const {
		Expect(TAG_INTARRAY);
		return intArray;
	}
	const std::string& GetString() const {
		Expect(TAG_STRING);
		return stringValue;
	}
	const std::vector<Tag>& GetList() const {
		Expect(TAG_LIST);
		return list;
	}
	const std::unordered_map<std::string, Tag>& GetCompound() const {
		Expect(TAG_COMPOUND);
		return compound;
	}

	// Compound lookup helpers
	bool Has(const std::string& _key) const {
		return compound.count(_key) > 0;
	}

	const Tag& Get(const std::string& _key) const {
		auto it = compound.find(_key);
		if (it == compound.end())
			throw std::runtime_error("NBT key not found: " + _key);
		return it->second;
	}

private:
	void Expect(TagType _t) const {
		if (type != _t)
			throw std::runtime_error("NBT type mismatch");
	}
};

struct NBTwriter {
	NBTwriter() = default;
	NBTwriter(std::vector<uint8_t>& _out, Tag& _root) {
		// root should be a TAG_Compound with whatever name you want (usually "")
		// writeTag handles type byte + name + payload + TAG_END automatically
		WriteTag(_out, _root);
	}

	int64_t WriteTag(std::vector<uint8_t>& _out, const Tag& _tag, bool _payload = false) {
		if (!_payload)
			_out.push_back(uint8_t(_tag.type));
		if (!_payload && _tag.type != TAG_END)
			WriteString(_out, _tag.name);

		switch (_tag.type) {
		case TAG_END:
			break;
		case TAG_BYTE:
			_out.push_back(uint8_t(_tag.byteValue));
			break;
		case TAG_SHORT:
			WriteI16(_out, _tag.shortValue);
			break;
		case TAG_INT:
			WriteI32(_out, _tag.intValue);
			break;
		case TAG_LONG:
			WriteI64(_out, _tag.longValue);
			break;
		case TAG_FLOAT:
			WriteF32(_out, _tag.floatValue);
			break;
		case TAG_DOUBLE:
			WriteF64(_out, _tag.doubleValue);
			break;
		case TAG_STRING:
			WriteString(_out, _tag.stringValue);
			break;

		case TAG_BYTEARRAY: {
			WriteI32(_out, int32_t(_tag.byteArray.size()));
			for (int8_t b : _tag.byteArray)
				_out.push_back(uint8_t(b));
			break;
		}

		case TAG_INTARRAY: {
			WriteI32(_out, int32_t(_tag.intArray.size()));
			for (int32_t b : _tag.intArray)
				WriteI32(_out, b);
			break;
		}

		case TAG_LIST: {
			WriteI8(_out, int8_t(_tag.listType));
			WriteI32(_out, int32_t(_tag.list.size()));
			for (const Tag& element : _tag.list)
				WriteTag(_out, element, true);
			break;
		}

		case TAG_COMPOUND: {
			for (const auto& [key, child] : _tag.compound)
				WriteTag(_out, child);
			// TAG_END terminates the compound
			_out.push_back(uint8_t(TAG_END));
			break;
		}

		default:
			throw std::runtime_error("Unknown tag type: " + std::to_string(_tag.type));
		}

		return 0;
	}

	// Write helpers
	void WriteI32(std::vector<uint8_t>& _out, int32_t _v) {
		uint32_t u = uint32_t(_v);
		_out.push_back((u >> 24) & 0xFF);
		_out.push_back((u >> 16) & 0xFF);
		_out.push_back((u >> 8) & 0xFF);
		_out.push_back(u & 0xFF);
	}

	void WriteI64(std::vector<uint8_t>& _out, int64_t _v) {
		WriteI32(_out, int32_t((uint64_t(_v) >> 32) & 0xFFFFFFFF));
		WriteI32(_out, int32_t((uint64_t(_v) & 0xFFFFFFFF)));
	}

	void WriteI16(std::vector<uint8_t>& _out, int16_t _v) {
		uint16_t u = uint16_t(_v);
		_out.push_back((u >> 8) & 0xFF);
		_out.push_back(u & 0xFF);
	}

	void WriteI8(std::vector<uint8_t>& _out, int8_t _v) {
		_out.push_back(uint8_t(_v));
	}

	void WriteF32(std::vector<uint8_t>& _out, float _v) {
		uint32_t raw;
		memcpy(&raw, &_v, 4);
		WriteI32(_out, int32_t(raw));
	}

	void WriteF64(std::vector<uint8_t>& _out, double _v) {
		uint64_t raw;
		memcpy(&raw, &_v, 8);
		WriteI64(_out, int64_t(raw));
	}

	void WriteString(std::vector<uint8_t>& _out, const std::string& _s) {
		WriteI16(_out, int16_t(_s.size()));
		_out.insert(_out.end(), _s.begin(), _s.end());
	}
};

struct NBTParser {
	uint8_t* data;
	int64_t length;
	int64_t pos;
	Tag root;

	NBTParser() = default;
	NBTParser(uint8_t* _pdata, int64_t _plength) : data(_pdata), length(_plength), pos(0) {
		root = ParseTag();
		if (root.type != TAG_COMPOUND)
			throw std::runtime_error("NBT root tag is not a compound!");
	}

	// Parse a tag, either with type and name bytes (parseTag) or just a payload (parsePayload)
	Tag ParsePayload(TagType _ptype, const std::string& _pname = "") {
		Tag tag{ _ptype, _pname, {} };

		switch (_ptype) {
		case TAG_BYTE:
			tag.byteValue = ReadI8();
			break;
		case TAG_SHORT:
			tag.shortValue = ReadI16();
			break;
		case TAG_INT:
			tag.intValue = ReadI32();
			break;
		case TAG_LONG:
			tag.longValue = ReadI64();
			break;
		case TAG_FLOAT:
			tag.floatValue = ReadF32();
			break;
		case TAG_DOUBLE:
			tag.doubleValue = ReadF64();
			break;
		case TAG_STRING:
			tag.stringValue = ReadString();
			break;

		case TAG_BYTEARRAY: {
			int32_t count = ReadI32();
			tag.byteArray.reserve(size_t(count));
			for (int i = 0; i < count; i++)
				tag.byteArray.push_back(ReadI8());
			break;
		}

		case TAG_INTARRAY: {
			int32_t count = ReadI32();
			tag.intArray.reserve(size_t(count));
			for (int i = 0; i < count; i++)
				tag.intArray.push_back(ReadI32());
			break;
		}

		case TAG_LIST: {
			int8_t innerType = ReadI8();
			int32_t count = ReadI32();

			if (innerType == TAG_END && count > 0)
				throw std::runtime_error("Invalid TAG_List");

			tag.list.reserve(size_t(count));
			for (int i = 0; i < count; i++)
				tag.list.push_back(ParsePayload(TagType(innerType)));

			tag.listType = TagType(innerType);
			break;
		}

		case TAG_COMPOUND: {
			while (true) {
				Tag child = ParseTag();
				if (child.type == TAG_END)
					break;
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
	Tag ParseTag() {
		if (pos >= length)
			throw std::runtime_error("Unexpected end of NBT data");

		TagType type = TagType(data[pos++]);
		if (type == TAG_END)
			return Tag{ TAG_END, "", {} }; // no name for TAG_End

		std::string name = ReadString();
		return ParsePayload(type, name);
	}

	// Read helpers
	int32_t ReadI32() {
		if (pos + 4 > length)
			throw std::runtime_error("NBT: unexpected end");
		uint32_t v = (uint32_t(data[pos]) << 24) | (uint32_t(data[pos + 1]) << 16) | (uint32_t(data[pos + 2]) << 8) |
		             (uint32_t(data[pos + 3]));
		pos += 4;
		return int32_t(v);
	}

	int64_t ReadI64() {
		uint32_t hi = uint32_t(ReadI32());
		uint32_t lo = uint32_t(ReadI32());
		return int64_t((uint64_t(hi) << 32) | lo);
	}

	int16_t ReadI16() {
		if (pos + 2 > length)
			throw std::runtime_error("NBT: i16 out of bounds");
		uint16_t v = static_cast<uint16_t>((static_cast<uint16_t>(data[pos]) << 8) |
		                                   static_cast<uint16_t>(data[pos + 1]));
		pos += 2;
		return int16_t(v);
	}

	int8_t ReadI8() {
		if (pos >= length)
			throw std::runtime_error("NBT: i8 out of bounds");
		return int8_t(data[pos++]);
	}

	float ReadF32() {
		uint32_t raw = uint32_t(ReadI32());
		float f;
		memcpy(&f, &raw, 4);
		return f;
	}

	double ReadF64() {
		uint64_t raw = uint64_t(ReadI64());
		double d;
		memcpy(&d, &raw, 8);
		return d;
	}

	std::string ReadString() {
		uint16_t len = uint16_t(ReadI16());
		if (pos + len > length)
			throw std::runtime_error("NBT: string out of bounds");
		std::string s(reinterpret_cast<const char*>(data) + pos, len);
		pos += len;
		return s;
	}
};