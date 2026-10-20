// Force-included into every translation unit of the PS2 build.
// Fills gaps in the PS2 newlib/libstdc++ environment without touching
// the portable code paths used on other platforms.
#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <format>
#include <iomanip>
#include <ostream>
#include <string>
#include <sys/file.h>
#include <optional>

// --- flock(): single process, nothing else can contend for the lock ---------
#ifndef LOCK_EX
#define LOCK_SH 1
#define LOCK_EX 2
#define LOCK_NB 4
#define LOCK_UN 8
#endif
static inline int ps2_flock(int, int) { return 0; }
#define flock ps2_flock

// --- std::put_time: not provided by this libstdc++ build --------------------
namespace std {
struct __ps2_put_time {
	const tm* t;
	const char* fmt;
};
inline __ps2_put_time put_time(const tm* t, const char* fmt) { return {t, fmt}; }
inline ostream& operator<<(ostream& os, const __ps2_put_time& p) {
	char buf[128];
	size_t n = strftime(buf, sizeof(buf), p.fmt, p.t);
	return os.write(buf, static_cast<streamsize>(n));
}
} // namespace std

// --- std::format for float/double (libstdc++ here lacks floating to_chars) --
namespace ps2_fmt {
struct FloatSpec {
	int width = 0;
	int precision = -1;
	char type = 0;
	char sign = '-';
	bool zero = false;
	char align = 0;
	char fill = ' ';
	bool alt = false;
};

template <class It>
constexpr It ParseSpec(It it, It end, FloatSpec& s) {
	if (it == end || *it == '}')
		return it;
	auto isAlign = [](char c) { return c == '<' || c == '>' || c == '^'; };
	if (it + 1 != end && isAlign(*(it + 1))) {
		s.fill = *it;
		s.align = *(it + 1);
		it += 2;
	} else if (isAlign(*it)) {
		s.align = *it++;
	}
	if (it != end && (*it == '+' || *it == '-' || *it == ' '))
		s.sign = *it++;
	if (it != end && *it == '#') {
		s.alt = true;
		++it;
	}
	if (it != end && *it == '0') {
		s.zero = true;
		++it;
	}
	while (it != end && *it >= '0' && *it <= '9')
		s.width = s.width * 10 + (*it++ - '0');
	if (it != end && *it == '.') {
		++it;
		s.precision = 0;
		while (it != end && *it >= '0' && *it <= '9')
			s.precision = s.precision * 10 + (*it++ - '0');
	}
	if (it != end && (*it == 'f' || *it == 'F' || *it == 'e' || *it == 'E' || *it == 'g' || *it == 'G'))
		s.type = *it++;
	return it;
}

inline std::string FormatDouble(double v, const FloatSpec& s) {
	char buf[400];
	if (s.type == 0 && s.precision < 0) {
		// Default: shortest representation that round-trips
		for (int p = 1; p <= 17; ++p) {
			std::snprintf(buf, sizeof(buf), "%.*g", p, v);
			if (std::strtod(buf, nullptr) == v)
				break;
		}
		// std::format switches to exponent form for very large/small values; %g does the same.
	} else {
		char t = s.type ? s.type : 'g';
		char f[16];
		std::snprintf(f, sizeof(f), "%%%s.*%c", s.alt ? "#" : "", t);
		std::snprintf(buf, sizeof(buf), f, s.precision < 0 ? 6 : s.precision, v);
	}
	std::string out = buf;
	if (s.sign == '+' && !out.empty() && out[0] != '-')
		out.insert(out.begin(), '+');
	else if (s.sign == ' ' && !out.empty() && out[0] != '-')
		out.insert(out.begin(), ' ');
	if (static_cast<int>(out.size()) < s.width) {
		size_t pad = static_cast<size_t>(s.width) - out.size();
		if (s.zero && !s.align) {
			size_t pos = (!out.empty() && (out[0] == '-' || out[0] == '+' || out[0] == ' ')) ? 1 : 0;
			out.insert(pos, pad, '0');
		} else if (s.align == '<') {
			out.append(pad, s.fill);
		} else if (s.align == '^') {
			out.insert(0, pad / 2, s.fill);
			out.append(pad - pad / 2, s.fill);
		} else {
			out.insert(0, pad, s.fill); // numbers right-align by default
		}
	}
	return out;
}
} // namespace ps2_fmt

namespace std {
template <>
struct formatter<double, char> {
	ps2_fmt::FloatSpec spec;
	constexpr auto parse(format_parse_context& ctx) { return ps2_fmt::ParseSpec(ctx.begin(), ctx.end(), spec); }
	template <class Ctx>
	auto format(double v, Ctx& ctx) const {
		auto s = ps2_fmt::FormatDouble(v, spec);
		return std::copy(s.begin(), s.end(), ctx.out());
	}
};
template <>
struct formatter<float, char> {
	ps2_fmt::FloatSpec spec;
	constexpr auto parse(format_parse_context& ctx) { return ps2_fmt::ParseSpec(ctx.begin(), ctx.end(), spec); }
	template <class Ctx>
	auto format(float v, Ctx& ctx) const {
		// Format via double but use float-shortest digits for the default spec
		ps2_fmt::FloatSpec sp = spec;
		std::string s;
		if (sp.type == 0 && sp.precision < 0) {
			char buf[64];
			for (int p = 1; p <= 9; ++p) {
				std::snprintf(buf, sizeof(buf), "%.*g", p, static_cast<double>(v));
				if (static_cast<float>(std::strtod(buf, nullptr)) == v)
					break;
			}
			s = buf;
		} else {
			s = ps2_fmt::FormatDouble(static_cast<double>(v), sp);
		}
		return std::copy(s.begin(), s.end(), ctx.out());
	}
};
} // namespace std
