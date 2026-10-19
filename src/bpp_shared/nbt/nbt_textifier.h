/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "logger.h"
#include "nbt_tag.h"
#include <cctype>

namespace {
enum TokenIdentifier : uint8_t {
	COMPOUND_BEGIN, // {
	COMPOUND_END,   // }
	LIST_BEGIN,     // [
	LIST_END,       // ]
	COLON, // :
	COMMA, // , 
	ARRAY_PREFIX, // B; I; L;
	STRING_LITERAL,
	NUMBER_LITERAL, // e.g., "1b", "300s", "1.5f", "42"
	INVALID
};

struct Token {
	TokenIdentifier identifier = INVALID;
	
	// This isn't just a char because tokens can be strings
	std::string tokenText = "";
};
}

struct NBTTextifier {
	// Converts an NBT compound to a string
	
};

struct TextNBTifier {
	// Converts a string to an NBT compound
	std::string nbtString;
	int64_t stepper = 0;

	void RunTest() {
		nbtString = R"---({
						"name": "Alex",
						"cool array": [
							["apple", "banana"],
							["carrot"]
						],
						"weapon_stats": {
							"id": minecraft:diamond_sword,
							"damage": 7.5f,
							"crit_chance": 0.2d,
							"durability": 1561s,
							"is_broken": 0b
						},
						"custom_array": [I; 550, -200, 1000],
						"escaped_text": "Line 1 \\\\ Line 2 with \"quotes\""
					})---";

		stepper = 0;
		std::vector<Token> tokens;
		tokens.clear();
		Lex(&tokens, stepper);

		for (auto& token : tokens) {
			Logger::Debug << "identifier: " << uint8_t(token.identifier) << " | Text: " << token.tokenText << "\n";
		}
	}

private:
	bool IsArrayIdentifier() {
		const char lowerChar = std::tolower(Peek());
		SkipWhitespace();
		Advance();
		const char higherChar = std::tolower(Peek());
		bool isArrayIdentifier = higherChar == ';' && (lowerChar == 'b' || lowerChar == 'i');
		if (!isArrayIdentifier)
			stepper--;
		return isArrayIdentifier;
	}

	void Lex(std::vector<Token>* _tokens, int64_t& _stepper) {
		while (IndexInBounds(stepper)) {
			SkipWhitespace();
			LexToken(_tokens, _stepper);
		}
	}

	void LexToken(std::vector<Token>* _tokens, int64_t& _stepper) {
		if (Peek() == '{') {
			LexCompound(_tokens, _stepper);
		}
		if (Peek() == '}' || Peek() == ']')
			return;

		Advance();
	}

	void LexList(std::vector<Token>* _tokens, int64_t& _stepper) {
		bool isSpecialList = false;
		TagType listType = TAG_END;
		int64_t length = 0;

		if (Peek() != '[') {
			Throw("Tried to parse an invalid list!");
		}

		_tokens->push_back({ .identifier = LIST_BEGIN, .tokenText = std::string(1, Peek()) });
		Advance();
		SkipWhitespace();
		isSpecialList = IsArrayIdentifier();

		while (IndexInBounds(stepper) && Peek() != ']') {
			SkipWhitespace();
			LexToken(_tokens, _stepper);
		}

		if (Peek() != ']') {
			Throw("Missing closing brace for list!");
		}

		_tokens->push_back({ .identifier = LIST_END, .tokenText = std::string(1, Peek()) });
		Advance();
	}

	void LexCompound(std::vector<Token>* _tokens, int64_t& _stepper) {
		if (Peek() != '{') {
			Throw("Tried to parse an invalid compound!");
		}

		_tokens->push_back({ .identifier = COMPOUND_BEGIN, .tokenText = std::string(1, Peek()) });
		Advance();

		while (IndexInBounds(stepper) && Peek() != '}') {
			SkipWhitespace();
			LexToken(_tokens, _stepper);
		}

		if (Peek() != '}') {
			Throw("Missing closing brace for compound!");
		}

		_tokens->push_back({ .identifier = COMPOUND_END, .tokenText = std::string(1, Peek()) });
		Advance();
	}

	// NOTE: These don't have safety! Be careful!
	void Throw(std::string _reason) const {
		Logger::Error << _reason << "\n";
		throw std::runtime_error(_reason);
	}

	void Warn(std::string _reason) const {
		Logger::Warning << _reason << "\n";
	}

	bool IndexInBounds(int64_t _index) const {
		return _index < nbtString.length();
	}
	
	char Peek() {
		if (!IndexInBounds(stepper))
			return '\0';
		return nbtString[stepper];
	}

	char PeekNext() {
		if (!IndexInBounds(stepper + 1))
			return '\0';
		return nbtString[stepper + 1];
	}

	// Seeks to the index in the string and sets the stepper
	char Seek(int64_t _index) {
		stepper = _index;
		return Peek();
	}

	// Advances the stepper
	void Advance() {
		stepper++;
	}

	// Skips until a non-whitespace character is provided
	void SkipWhitespace() {
		while (std::isspace(nbtString[stepper])) {
			stepper++;
		}
	}
};