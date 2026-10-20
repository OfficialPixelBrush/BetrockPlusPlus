// Minimal stand-in for replxx on the PS2 (no terminal / line editing there).
// Only the API surface used by ServerConsole is provided; input() never yields.
#pragma once
#include <functional>
#include <string>
#include <vector>

namespace replxx {
class Replxx {
public:
	enum class ACTION { COMPLETE_LINE };
	enum class ACTION_RESULT { CONTINUE, RETURN, BAIL };
	struct KEY {
		static constexpr char32_t TAB = 9;
		static constexpr char32_t control(char32_t c) { return c & 0x1f; }
		static constexpr char32_t meta(char32_t c) { return c | 0x4000000; }
	};
	struct Completion {
		std::string text;
		Completion(std::string t) : text(std::move(t)) {}
		Completion(const char* t) : text(t) {}
	};
	using completions_t = std::vector<Completion>;
	using key_press_handler_t = std::function<ACTION_RESULT(char32_t)>;
	void bind_key(char32_t, key_press_handler_t) {}
	ACTION_RESULT invoke(ACTION, char32_t) { return ACTION_RESULT::CONTINUE; }
	void emulate_key_press(char32_t) {}
	void set_completion_callback(std::function<completions_t(const std::string&, int&)>) {}
	void write(const char*, int) {}
	const char* input(const std::string&) { return nullptr; }
};
} // namespace replxx
