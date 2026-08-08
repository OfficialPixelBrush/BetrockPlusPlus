/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "logger.h"
#include <cstdint>
#include <cstring>
#include <format>
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

	// Typed setter
	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	void Set(T _value) {
		switch (type) {
		case TAG_BYTE:
			byteValue = _value;
			break;
		case TAG_SHORT:
			shortValue = _value;
			break;
		case TAG_INT:
			intValue = _value;
			break;
		case TAG_LONG:
			longValue = _value;
			break;
		case TAG_FLOAT:
			floatValue = _value;
			break;
		case TAG_DOUBLE:
			doubleValue = _value;
			break;
		default:
			GlobalLogger().warn << "Tried to use numeric setter on non-numeric NBT type " << type << "!\n";
			break;
		}
	}

	void Set(const std::string& _value) {
		if (type == TAG_STRING)
			stringValue = _value;
		else
			GlobalLogger().warn << "Tried to use string setter on non-string NBT type " << type << "!\n";
	}

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
		return compound.contains(_key);
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
	size_t pos;
	NBTwriter() = default;
	NBTwriter(std::vector<uint8_t>& _out, Tag& _root) : pos(0) {
		// root should be a TAG_Compound with whatever name you want (usually "")
		// writeTag handles type byte + name + payload + TAG_END automatically
		_out.resize(WriteTag(_out, _root, false, true));
		WriteTag(_out, _root, false, false);
	}

	size_t WriteTag(std::vector<uint8_t>& _out, const Tag& _tag, const bool _payload = false,
	                const bool _dryRun = false) {
		size_t size = 0;

		if (!_payload) {
			size += sizeof(uint8_t);
			if (!_dryRun)
				_out[pos++] = (uint8_t(_tag.type));
		}
		if (!_payload && _tag.type != TAG_END)
			size += WriteString(_out, _tag.name, _dryRun);

		switch (_tag.type) {
		case TAG_END:
			break;
		case TAG_BYTE:
			size += WriteI8(_out, _tag.byteValue, _dryRun);
			break;
		case TAG_SHORT:
			size += WriteI16(_out, _tag.shortValue, _dryRun);
			break;
		case TAG_INT:
			size += WriteI32(_out, _tag.intValue, _dryRun);
			break;
		case TAG_LONG:
			size += WriteI64(_out, _tag.longValue, _dryRun);
			break;
		case TAG_FLOAT:
			size += WriteF32(_out, _tag.floatValue, _dryRun);
			break;
		case TAG_DOUBLE:
			size += WriteF64(_out, _tag.doubleValue, _dryRun);
			break;
		case TAG_STRING:
			size += WriteString(_out, _tag.stringValue, _dryRun);
			break;

		case TAG_BYTEARRAY: {
			size += WriteI32(_out, int32_t(_tag.byteArray.size()), _dryRun);
			size += _tag.byteArray.size();
			if (_dryRun)
				break;
			// TODO: Assume we got the size
			memcpy(_out.data() + pos, _tag.byteArray.data(), _tag.byteArray.size());

			pos += _tag.byteArray.size();
			break;
		}

		case TAG_INTARRAY: {
			size += WriteI32(_out, int32_t(_tag.intArray.size()), _dryRun);
			size += _tag.intArray.size() * sizeof(int32_t);
			if (_dryRun)
				break;
			for (const int32_t b : _tag.intArray) {
				uint32_t u = uint32_t(b);

				_out[pos + 0] = u >> 24;
				_out[pos + 1] = u >> 16;
				_out[pos + 2] = u >> 8;
				_out[pos + 3] = u;

				pos += 4;
			}
			break;
		}

		case TAG_LIST: {
			size += WriteI8(_out, int8_t(_tag.listType), _dryRun);
			size += WriteI32(_out, int32_t(_tag.list.size()), _dryRun);
			for (const Tag& element : _tag.list)
				size += WriteTag(_out, element, true, _dryRun);
			break;
		}

		case TAG_COMPOUND: {
			for (const auto& [key, child] : _tag.compound)
				size += WriteTag(_out, child, false, _dryRun);
			// TAG_END terminates the compound
			size += WriteI8(_out, uint8_t(TAG_END), _dryRun);
			break;
		}

		default:
			throw std::runtime_error("Unknown tag type: " + std::to_string(_tag.type));
		}

		return size;
	}

	// Write helpers
	inline size_t WriteI8(std::vector<uint8_t>& _out, const int8_t _v, const bool _dryRun = false) {
		if (!_dryRun)
			_out[pos++] = (uint8_t(_v));
		return sizeof(int8_t);
	}

	inline size_t WriteI16(std::vector<uint8_t>& _out, const int16_t _v, const bool _dryRun = false) {
		if (!_dryRun) {
			uint16_t u = uint16_t(_v);
			_out[pos] = ((u >> 8) & 0xFF);
			_out[pos + 1] = (u & 0xFF);
			pos += sizeof(int16_t);
		}
		return sizeof(int16_t);
	}

	inline size_t WriteI32(std::vector<uint8_t>& _out, const int32_t _v, const bool _dryRun = false) {
		if (!_dryRun) {
			uint32_t u = uint32_t(_v);
			_out[pos] = ((u >> 24) & 0xFF);
			_out[pos + 1] = ((u >> 16) & 0xFF);
			_out[pos + 2] = ((u >> 8) & 0xFF);
			_out[pos + 3] = (u & 0xFF);
			pos += sizeof(int32_t);
		}
		return sizeof(int32_t);
	}

	inline size_t WriteI64(std::vector<uint8_t>& _out, const int64_t _v, const bool _dryRun = false) {
		if (!_dryRun) {
			uint64_t u = uint64_t(_v);
			_out[pos] = ((u >> 56) & 0xFF);
			_out[pos + 1] = ((u >> 48) & 0xFF);
			_out[pos + 2] = ((u >> 40) & 0xFF);
			_out[pos + 3] = ((u >> 32) & 0xFF);
			_out[pos + 4] = ((u >> 24) & 0xFF);
			_out[pos + 5] = ((u >> 16) & 0xFF);
			_out[pos + 6] = ((u >> 8) & 0xFF);
			_out[pos + 7] = (u & 0xFF);
			pos += sizeof(int64_t);
		}
		return sizeof(int64_t);
	}

	inline size_t WriteF32(std::vector<uint8_t>& _out, const float _v, const bool _dryRun = false) {
		if (!_dryRun) {
			uint32_t raw = std::bit_cast<uint32_t>(_v);
			WriteI32(_out, int32_t(raw));
		}
		return sizeof(float);
	}

	inline size_t WriteF64(std::vector<uint8_t>& _out, const double _v, const bool _dryRun = false) {
		if (!_dryRun) {
			uint64_t raw = std::bit_cast<uint64_t>(_v);
			WriteI64(_out, int64_t(raw));
		}
		return sizeof(double);
	}

	inline size_t WriteString(std::vector<uint8_t>& _out, const std::string& _s, const bool _dryRun = false) {
		if (!_dryRun) {
			WriteI16(_out, int16_t(_s.size()));
			memcpy(_out.data() + pos, _s.data(), _s.size());
			pos += _s.size();
		}
		return sizeof(int16_t) + _s.size();
	}
};

struct NBTParser {
	uint8_t* data;
	size_t length;
	size_t pos;
	Tag root;

	NBTParser() = default;
	NBTParser(uint8_t* _pdata, size_t _plength) : data(_pdata), length(_plength), pos(0) {
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
			tag.byteArray.resize(count);
			if (count < 0 || pos + size_t(count) > length) [[unlikely]]
				throw std::runtime_error("NBT: byte array out of bounds");
			std::memcpy(tag.byteArray.data(), data + pos, count);
			pos += count;
			break;
		}

		case TAG_INTARRAY: {
			int32_t count = ReadI32();
			tag.intArray.resize(count);
			for (auto& v : tag.intArray)
				v = ReadI32();
			break;
		}

		case TAG_LIST: {
			int8_t innerType = ReadI8();
			int32_t count = ReadI32();

			if (innerType == TAG_END && count > 0)
				throw std::runtime_error("Invalid TAG_List");

			tag.list.reserve(size_t(count));
			for (int i = 0; i < count; i++)
				tag.list.emplace_back(ParsePayload(TagType(innerType)));

			tag.listType = TagType(innerType);
			break;
		}

		case TAG_COMPOUND: {
			while (true) {
				Tag child = ParseTag();
				if (child.type == TAG_END)
					break;
				tag.compound.try_emplace(child.name, std::move(child));
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

	inline int8_t ReadI8() {
		if (pos >= length)
			throw std::runtime_error("NBT: i8 out of bounds");
		return int8_t(data[pos++]);
	}

	inline int16_t ReadI16() {
		if (pos + 2 > length)
			throw std::runtime_error("NBT: i16 out of bounds");
		uint16_t v = static_cast<uint16_t>((static_cast<uint16_t>(data[pos]) << 8) |
		                                   static_cast<uint16_t>(data[pos + 1]));
		pos += 2;
		return int16_t(v);
	}

	inline int32_t ReadI32() {
		if (pos + 4 > length)
			throw std::runtime_error("NBT: unexpected end");
		uint32_t v = (uint32_t(data[pos]) << 24) | (uint32_t(data[pos + 1]) << 16) | (uint32_t(data[pos + 2]) << 8) |
		             (uint32_t(data[pos + 3]));
		pos += 4;
		return int32_t(v);
	}

	inline int64_t ReadI64() {
		uint64_t hi = uint32_t(ReadI32());
		return (hi << 32) | uint32_t(ReadI32());
	}

	inline float ReadF32() {
		uint32_t raw = uint32_t(ReadI32());
		return std::bit_cast<float>(raw);
	}

	inline double ReadF64() {
		uint64_t raw = uint64_t(ReadI64());
		return std::bit_cast<double>(raw);
	}

	inline std::string ReadString() {
		uint16_t len = uint16_t(ReadI16());
		if (pos + len > length)
			throw std::runtime_error(std::format("NBT: string out of bounds ({}+{}/{})", pos, len, length));
		std::string s(reinterpret_cast<const char*>(data) + pos, len);
		pos += len;
		return s;
	}
};