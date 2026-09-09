/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include "logger.h"
#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstring>
#include <expected>
#include <format>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace nbt {

enum class TagType : uint8_t {
	End = 0,
	Byte,
	Short,
	Int,
	Long,
	Float,
	Double,
	ByteArray,
	String,
	List,
	Compound,
	IntArray,
};

[[nodiscard]] constexpr auto to_string(TagType type) noexcept -> std::string_view {
	using enum TagType;
	switch (type) {
	case End:
		return "TAG_End";
	case Byte:
		return "TAG_Byte";
	case Short:
		return "TAG_Short";
	case Int:
		return "TAG_Int";
	case Long:
		return "TAG_Long";
	case Float:
		return "TAG_Float";
	case Double:
		return "TAG_Double";
	case ByteArray:
		return "TAG_Byte_Array";
	case String:
		return "TAG_String";
	case List:
		return "TAG_List";
	case Compound:
		return "TAG_Compound";
	case IntArray:
		return "TAG_Int_Array";
	default:
		return "TAG_Unknown";
	}
}

class Tag;
struct CompoundEntry;

using Byte = int8_t;
using Short = int16_t;
using Int = int32_t;
using Long = int64_t;
using Float = float;
using Double = double;
using ByteArray = std::vector<int8_t>;
using IntArray = std::vector<int32_t>;
using String = std::string;
using List = std::vector<Tag>;
// vector<pair>, not a map: compounds are small, so a linear scan beats a tree/hash
// map on both memory and cache behaviour, and keeps sizeof(Tag) down to its largest
// leaf payload (std::string) rather than a map header.
using Compound = std::vector<CompoundEntry>;

class Tag {
public:
	using Value = std::variant<std::monostate, Byte, Short, Int, Long, Float, Double, ByteArray, String, List,
	                            Compound, IntArray>;

	TagType type = TagType::End;
	TagType list_type = TagType::End; // element type; only meaningful when type == List
	Value value;

	Tag() = default;

	// ---- Factories ----
	[[nodiscard]] static auto byte(Byte v) -> Tag { return leaf(TagType::Byte, v); }
	[[nodiscard]] static auto short_(Short v) -> Tag { return leaf(TagType::Short, v); }
	[[nodiscard]] static auto int_(Int v) -> Tag { return leaf(TagType::Int, v); }
	[[nodiscard]] static auto long_(Long v) -> Tag { return leaf(TagType::Long, v); }
	[[nodiscard]] static auto float_(Float v) -> Tag { return leaf(TagType::Float, v); }
	[[nodiscard]] static auto double_(Double v) -> Tag { return leaf(TagType::Double, v); }
	[[nodiscard]] static auto byte_array(ByteArray v) -> Tag { return leaf(TagType::ByteArray, std::move(v)); }
	[[nodiscard]] static auto int_array(IntArray v) -> Tag { return leaf(TagType::IntArray, std::move(v)); }
	[[nodiscard]] static auto string(String v) -> Tag { return leaf(TagType::String, std::move(v)); }

	[[nodiscard]] static auto list(TagType element_type = TagType::End) -> Tag {
		Tag tag;
		tag.type = TagType::List;
		tag.list_type = element_type;
		tag.value = List{};
		return tag;
	}

	[[nodiscard]] static auto compound() -> Tag {
		Tag tag;
		tag.type = TagType::Compound;
		tag.value = Compound{};
		return tag;
	}

	// ---- Fluent building ----
	Tag& add(Tag element); // append to a List, returns *this
	Tag& put(std::string name, Tag child); // insert/overwrite in a Compound, returns *this

	// ---- Compound lookup ----
	[[nodiscard]] auto has(std::string_view name) const noexcept -> bool { return find(name) != nullptr; }
	[[nodiscard]] auto find(std::string_view name) const noexcept -> const Tag*;
	[[nodiscard]] auto find(std::string_view name) noexcept -> Tag*;
	[[nodiscard]] auto at(std::string_view name) const -> const Tag&; // throws if missing

	[[nodiscard]] auto size() const noexcept -> size_t; // List/Compound/*Array/String length, else 0

	// Non-throwing typed access.
	template <typename T>
	[[nodiscard]] auto get() const noexcept -> const T* {
		return std::get_if<T>(&value);
	}

	// Throwing typed access, for call sites that already know the type.
	[[nodiscard]] auto as_byte() const -> Byte { return expect<Byte>(TagType::Byte); }
	[[nodiscard]] auto as_short() const -> Short { return expect<Short>(TagType::Short); }
	[[nodiscard]] auto as_int() const -> Int { return expect<Int>(TagType::Int); }
	[[nodiscard]] auto as_long() const -> Long { return expect<Long>(TagType::Long); }
	[[nodiscard]] auto as_float() const -> Float { return expect<Float>(TagType::Float); }
	[[nodiscard]] auto as_double() const -> Double { return expect<Double>(TagType::Double); }
	[[nodiscard]] auto as_byte_array() const -> const ByteArray& { return expect<ByteArray>(TagType::ByteArray); }
	[[nodiscard]] auto as_byte_array() -> ByteArray& { return expect<ByteArray>(TagType::ByteArray); }
	[[nodiscard]] auto as_int_array() const -> const IntArray& { return expect<IntArray>(TagType::IntArray); }
	[[nodiscard]] auto as_int_array() -> IntArray& { return expect<IntArray>(TagType::IntArray); }
	[[nodiscard]] auto as_string() const -> const String& { return expect<String>(TagType::String); }
	[[nodiscard]] auto as_string() -> String& { return expect<String>(TagType::String); }
	[[nodiscard]] auto as_list() const -> const List& { return expect<List>(TagType::List); }
	[[nodiscard]] auto as_list() -> List& { return expect<List>(TagType::List); }
	[[nodiscard]] auto as_compound() const -> const Compound& { return expect<Compound>(TagType::Compound); }
	[[nodiscard]] auto as_compound() -> Compound& { return expect<Compound>(TagType::Compound); }

	// Typed setters; warn (rather than throw) on a type mismatch.
	template <typename T>
	    requires std::is_arithmetic_v<T>
	void set(T v) {
		switch (type) {
		case TagType::Byte:
			value = static_cast<Byte>(v);
			break;
		case TagType::Short:
			value = static_cast<Short>(v);
			break;
		case TagType::Int:
			value = static_cast<Int>(v);
			break;
		case TagType::Long:
			value = static_cast<Long>(v);
			break;
		case TagType::Float:
			value = static_cast<Float>(v);
			break;
		case TagType::Double:
			value = static_cast<Double>(v);
			break;
		default:
			GlobalLogger().warn << "Tried to use numeric setter on non-numeric NBT type " << to_string(type) << "!\n";
			break;
		}
	}

	void set(std::string v) {
		if (type == TagType::String) {
			value = std::move(v);
		} else {
			GlobalLogger().warn << "Tried to use string setter on non-string NBT type " << to_string(type) << "!\n";
		}
	}

private:
	template <typename T>
	[[nodiscard]] static auto leaf(TagType t, T v) -> Tag {
		Tag tag;
		tag.type = t;
		tag.value = std::move(v);
		return tag;
	}

	template <typename T>
	[[nodiscard]] auto expect(TagType expected) const -> const T& {
		if (type != expected) {
			GlobalLogger().error << "Unexpected NBT type in getter! '" << to_string(expected) << "'. Expected: '"
			                      << to_string(type) << "'\n";
			throw std::runtime_error("Unexpected NBT type!");
		}
		return std::get<T>(value);
	}

	template <typename T>
	[[nodiscard]] auto expect(TagType expected) -> T& {
		if (type != expected) {
			GlobalLogger().error << "Unexpected NBT type in getter! '" << to_string(expected) << "'. Expected: '"
			                      << to_string(type) << "'\n";
			throw std::runtime_error("Unexpected NBT type!");
		}
		return std::get<T>(value);
	}
};

struct CompoundEntry {
	std::string name;
	Tag tag;
};

inline auto Tag::find(std::string_view name) const noexcept -> const Tag* {
	if (type != TagType::Compound)
		return nullptr;
	const auto& entries = std::get<Compound>(value);
	auto it = std::ranges::find(entries, name, &CompoundEntry::name);
	return it != entries.end() ? &it->tag : nullptr;
}

inline auto Tag::find(std::string_view name) noexcept -> Tag* {
	return const_cast<Tag*>(std::as_const(*this).find(name));
}

inline auto Tag::at(std::string_view name) const -> const Tag& {
	if (const Tag* found = find(name))
		return *found;
	GlobalLogger().error << "Tried to access a NBT tag that doesn't exist! (" << name << ")\n";
	throw std::runtime_error("NBT tag doesn't exist!");
}

inline auto Tag::size() const noexcept -> size_t {
	switch (type) {
	case TagType::ByteArray:
		return std::get<ByteArray>(value).size();
	case TagType::IntArray:
		return std::get<IntArray>(value).size();
	case TagType::String:
		return std::get<String>(value).size();
	case TagType::List:
		return std::get<List>(value).size();
	case TagType::Compound:
		return std::get<Compound>(value).size();
	default:
		return 0;
	}
}

inline Tag& Tag::add(Tag element) {
	if (type != TagType::List) {
		GlobalLogger().warn << "Tried to add an element to a non-list NBT tag!\n";
		return *this;
	}
	auto& elements = std::get<List>(value);
	if (elements.empty() && list_type == TagType::End)
		list_type = element.type;
	elements.push_back(std::move(element));
	return *this;
}

inline Tag& Tag::put(std::string name, Tag child) {
	if (type != TagType::Compound) {
		GlobalLogger().warn << "Tried to put a child into a non-compound NBT tag!\n";
		return *this;
	}
	auto& entries = std::get<Compound>(value);
	auto it = std::ranges::find(entries, name, &CompoundEntry::name);
	if (it != entries.end())
		it->tag = std::move(child);
	else
		entries.push_back({ std::move(name), std::move(child) });
	return *this;
}

// ---------------------------------------------------------------------------
// Binary encode / decode
// ---------------------------------------------------------------------------

// Writes `root` (a TAG_Compound) as big-endian NBT. Computes the exact encoded
// size first so the output buffer is allocated once, instead of growing it with
// push_back as we go.
class NBTWriter {
public:
	[[nodiscard]] static auto write(const Tag& root, std::string_view root_name = "") -> std::vector<uint8_t> {
		std::vector<uint8_t> out(tag_size(root, root_name));
		size_t pos = 0;
		write_tag(out, pos, root, root_name);
		return out;
	}

private:
	[[nodiscard]] static auto string_size(std::string_view s) -> size_t { return sizeof(uint16_t) + s.size(); }

	[[nodiscard]] static auto tag_size(const Tag& tag, std::string_view name) -> size_t {
		if (tag.type == TagType::End)
			return sizeof(uint8_t);
		return sizeof(uint8_t) + string_size(name) + payload_size(tag);
	}

	[[nodiscard]] static auto payload_size(const Tag& tag) -> size_t {
		switch (tag.type) {
		case TagType::End:
			return 0;
		case TagType::Byte:
			return sizeof(Byte);
		case TagType::Short:
			return sizeof(Short);
		case TagType::Int:
			return sizeof(Int);
		case TagType::Long:
			return sizeof(Long);
		case TagType::Float:
			return sizeof(Float);
		case TagType::Double:
			return sizeof(Double);
		case TagType::String:
			return string_size(std::get<String>(tag.value));
		case TagType::ByteArray:
			return sizeof(int32_t) + std::get<ByteArray>(tag.value).size();
		case TagType::IntArray:
			return sizeof(int32_t) + std::get<IntArray>(tag.value).size() * sizeof(int32_t);
		case TagType::List: {
			size_t size = sizeof(uint8_t) + sizeof(int32_t);
			for (const Tag& element : std::get<List>(tag.value))
				size += payload_size(element);
			return size;
		}
		case TagType::Compound: {
			size_t size = sizeof(uint8_t); // TAG_End terminator
			for (const auto& [name, child] : std::get<Compound>(tag.value))
				size += tag_size(child, name);
			return size;
		}
		default:
			throw std::runtime_error(std::format("Unknown NBT tag type: {}", uint8_t(tag.type)));
		}
	}

	static void write_tag(std::vector<uint8_t>& out, size_t& pos, const Tag& tag, std::string_view name) {
		out[pos++] = uint8_t(tag.type);
		if (tag.type == TagType::End)
			return;
		write_string(out, pos, name);
		write_payload(out, pos, tag);
	}

	static void write_payload(std::vector<uint8_t>& out, size_t& pos, const Tag& tag) {
		switch (tag.type) {
		case TagType::End:
			break;
		case TagType::Byte:
			write_i8(out, pos, std::get<Byte>(tag.value));
			break;
		case TagType::Short:
			write_i16(out, pos, std::get<Short>(tag.value));
			break;
		case TagType::Int:
			write_i32(out, pos, std::get<Int>(tag.value));
			break;
		case TagType::Long:
			write_i64(out, pos, std::get<Long>(tag.value));
			break;
		case TagType::Float:
			write_f32(out, pos, std::get<Float>(tag.value));
			break;
		case TagType::Double:
			write_f64(out, pos, std::get<Double>(tag.value));
			break;
		case TagType::String:
			write_string(out, pos, std::get<String>(tag.value));
			break;

		case TagType::ByteArray: {
			const auto& arr = std::get<ByteArray>(tag.value);
			write_i32(out, pos, int32_t(arr.size()));
			std::memcpy(out.data() + pos, arr.data(), arr.size());
			pos += arr.size();
			break;
		}

		case TagType::IntArray: {
			const auto& arr = std::get<IntArray>(tag.value);
			write_i32(out, pos, int32_t(arr.size()));
			for (int32_t v : arr)
				write_i32(out, pos, v);
			break;
		}

		case TagType::List: {
			const auto& elements = std::get<List>(tag.value);
			out[pos++] = uint8_t(tag.list_type);
			write_i32(out, pos, int32_t(elements.size()));
			for (const Tag& element : elements)
				write_payload(out, pos, element);
			break;
		}

		case TagType::Compound: {
			for (const auto& [name, child] : std::get<Compound>(tag.value))
				write_tag(out, pos, child, name);
			out[pos++] = uint8_t(TagType::End);
			break;
		}

		default:
			throw std::runtime_error(std::format("Unknown NBT tag type: {}", uint8_t(tag.type)));
		}
	}

	static void write_i8(std::vector<uint8_t>& out, size_t& pos, int8_t v) { out[pos++] = uint8_t(v); }

	static void write_i16(std::vector<uint8_t>& out, size_t& pos, int16_t v) {
		auto u = uint16_t(v);
		out[pos + 0] = uint8_t(u >> 8);
		out[pos + 1] = uint8_t(u);
		pos += sizeof(u);
	}

	static void write_i32(std::vector<uint8_t>& out, size_t& pos, int32_t v) {
		auto u = uint32_t(v);
		out[pos + 0] = uint8_t(u >> 24);
		out[pos + 1] = uint8_t(u >> 16);
		out[pos + 2] = uint8_t(u >> 8);
		out[pos + 3] = uint8_t(u);
		pos += sizeof(u);
	}

	static void write_i64(std::vector<uint8_t>& out, size_t& pos, int64_t v) {
		auto u = uint64_t(v);
		out[pos + 0] = uint8_t(u >> 56);
		out[pos + 1] = uint8_t(u >> 48);
		out[pos + 2] = uint8_t(u >> 40);
		out[pos + 3] = uint8_t(u >> 32);
		out[pos + 4] = uint8_t(u >> 24);
		out[pos + 5] = uint8_t(u >> 16);
		out[pos + 6] = uint8_t(u >> 8);
		out[pos + 7] = uint8_t(u);
		pos += sizeof(u);
	}

	static void write_f32(std::vector<uint8_t>& out, size_t& pos, float v) {
		write_i32(out, pos, std::bit_cast<int32_t>(v));
	}
	static void write_f64(std::vector<uint8_t>& out, size_t& pos, double v) {
		write_i64(out, pos, std::bit_cast<int64_t>(v));
	}

	static void write_string(std::vector<uint8_t>& out, size_t& pos, std::string_view s) {
		write_i16(out, pos, int16_t(s.size()));
		std::memcpy(out.data() + pos, s.data(), s.size());
		pos += s.size();
	}
};

enum class NBTError : uint8_t {
	UnexpectedEnd,
	InvalidTag,
	RootNotCompound,
};

struct NBTErrorInfo {
	NBTError error;
	std::string message;
	size_t position{ 0 };
};

// Parses big-endian NBT binary data into a Tag tree. Internally uses exceptions
// for malformed input (simplest for recursive descent), but they never escape
// this class: parse() converts them to std::expected at the boundary.
class NBTParser {
public:
	struct Named {
		std::string name;
		Tag tag;
	};

	[[nodiscard]] static auto parse(std::span<const uint8_t> data) -> std::expected<Tag, NBTErrorInfo> {
		auto result = parse_named(data);
		if (!result)
			return std::unexpected(result.error());
		return std::move(result->tag);
	}

	[[nodiscard]] static auto parse_named(std::span<const uint8_t> data) -> std::expected<Named, NBTErrorInfo> {
		try {
			NBTParser parser(data);
			auto [name, tag] = parser.parse_tag();
			if (tag.type != TagType::Compound) {
				return std::unexpected(
				    NBTErrorInfo{ .error = NBTError::RootNotCompound, .message = "NBT root tag is not a compound!" });
			}
			return Named{ std::move(name), std::move(tag) };
		} catch (const Failure& failure) {
			return std::unexpected(
			    NBTErrorInfo{ .error = failure.error, .message = failure.message, .position = failure.position });
		}
	}

private:
	struct Failure {
		NBTError error;
		std::string message;
		size_t position;
	};

	std::span<const uint8_t> data;
	size_t pos = 0;

	explicit NBTParser(std::span<const uint8_t> d) : data(d) {}

	[[noreturn]] void fail(NBTError error, std::string message) const {
		throw Failure{ error, std::move(message), pos };
	}

	void ensure(size_t n) const {
		if (pos + n > data.size())
			fail(NBTError::UnexpectedEnd, "Unexpected end of NBT data");
	}

	// Type byte + name + payload.
	auto parse_tag() -> std::pair<std::string, Tag> {
		ensure(1);
		auto type = TagType(data[pos++]);
		if (type == TagType::End)
			return { "", Tag{} };

		std::string name = read_string();
		return { std::move(name), parse_payload(type) };
	}

	// Payload only, used for list elements (no type byte or name).
	auto parse_payload(TagType type) -> Tag {
		Tag tag;
		tag.type = type;

		switch (type) {
		case TagType::End:
			break;
		case TagType::Byte:
			tag.value = read_i8();
			break;
		case TagType::Short:
			tag.value = read_i16();
			break;
		case TagType::Int:
			tag.value = read_i32();
			break;
		case TagType::Long:
			tag.value = read_i64();
			break;
		case TagType::Float:
			tag.value = read_f32();
			break;
		case TagType::Double:
			tag.value = read_f64();
			break;
		case TagType::String:
			tag.value = read_string();
			break;

		case TagType::ByteArray: {
			int32_t count = read_i32();
			if (count < 0)
				fail(NBTError::InvalidTag, "NBT: negative byte array length");
			ensure(static_cast<size_t>(count));
			ByteArray arr(static_cast<size_t>(count));
			std::memcpy(arr.data(), data.data() + pos, arr.size());
			pos += arr.size();
			tag.value = std::move(arr);
			break;
		}

		case TagType::IntArray: {
			int32_t count = read_i32();
			if (count < 0)
				fail(NBTError::InvalidTag, "NBT: negative int array length");
			IntArray arr(static_cast<size_t>(count));
			for (auto& v : arr)
				v = read_i32();
			tag.value = std::move(arr);
			break;
		}

		case TagType::List: {
			auto element_type = TagType(read_i8());
			int32_t count = read_i32();
			if (element_type == TagType::End && count > 0)
				fail(NBTError::InvalidTag, "Invalid TAG_List: TAG_End element type with nonzero length");
			if (count < 0)
				fail(NBTError::InvalidTag, "NBT: negative list length");

			List elements;
			elements.reserve(static_cast<size_t>(count));
			for (int32_t i = 0; i < count; ++i)
				elements.push_back(parse_payload(element_type));

			tag.list_type = element_type;
			tag.value = std::move(elements);
			break;
		}

		case TagType::Compound: {
			Compound entries;
			while (true) {
				auto [name, child] = parse_tag();
				if (child.type == TagType::End)
					break;
				entries.push_back({ std::move(name), std::move(child) });
			}
			tag.value = std::move(entries);
			break;
		}

		default:
			fail(NBTError::InvalidTag, std::format("Unknown NBT tag type: {}", uint8_t(type)));
		}

		return tag;
	}

	auto read_i8() -> int8_t {
		ensure(1);
		return int8_t(data[pos++]);
	}

	auto read_i16() -> int16_t {
		ensure(2);
		auto v = uint16_t((uint16_t(data[pos]) << 8) | uint16_t(data[pos + 1]));
		pos += 2;
		return int16_t(v);
	}

	auto read_i32() -> int32_t {
		ensure(4);
		auto v = (uint32_t(data[pos]) << 24) | (uint32_t(data[pos + 1]) << 16) | (uint32_t(data[pos + 2]) << 8) |
		         uint32_t(data[pos + 3]);
		pos += 4;
		return int32_t(v);
	}

	auto read_i64() -> int64_t {
		auto hi = uint64_t(uint32_t(read_i32()));
		auto lo = uint64_t(uint32_t(read_i32()));
		return int64_t((hi << 32) | lo);
	}

	auto read_f32() -> float { return std::bit_cast<float>(read_i32()); }
	auto read_f64() -> double { return std::bit_cast<double>(read_i64()); }

	auto read_string() -> std::string {
		auto len = uint16_t(read_i16());
		ensure(len);
		std::string s(reinterpret_cast<const char*>(data.data()) + pos, len);
		pos += len;
		return s;
	}
};

} // namespace nbt
