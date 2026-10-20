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

struct NBTTextifier {
	// Converts an NBT compound to a string
	
};

struct TextNBTifier {
	// Converts a string to an NBT compound
	std::string nbtString;
	int64_t stepper = 0;

	Tag Parse(std::string _string) {
		nbtString = _string;
		stepper = 0;
		std::vector<Token> tokens;
		tokens.clear();
		Lex(&tokens, stepper);

		stepper = 0;
		return Link(&tokens);
	}

private:
	enum TokenIdentifier : uint8_t {
		COMPOUND_BEGIN, // {
		COMPOUND_END,   // }
		LIST_BEGIN,     // [
		LIST_END,       // ]
		COLON,          // :
		COMMA,          // ,
		ARRAY_PREFIX,   // B; I; L;
		STRING_LITERAL,
		NUMBER_LITERAL, // e.g., "1b", "300s", "1.5f", "42"
		INVALID
	};

	struct Token {
		TokenIdentifier identifier = INVALID;
		std::string tokenText = "";
		TagType type = TAG_END;
	};

	// Linker
	Tag Link(std::vector<Token>* _tokens);
	Tag LinkTag(std::vector<Token>* _tokens, int64_t& _stepper, std::string& _tagName);
	Tag LinkList(std::vector<Token>* _tokens, int64_t& _stepper, std::string _name);
	Tag LinkCompound(std::vector<Token>* _tokens, int64_t& _stepper, std::string _name);
	void SetNumberFromText(Tag& _tag, std::string _text);

	// Lexer
	void Lex(std::vector<Token>* _tokens, int64_t& _stepper);
	void LexToken(std::vector<Token>* _tokens, int64_t& _stepper);
	void LexLiteral(std::vector<Token>* _tokens, int64_t& _stepper);
	void LexList(std::vector<Token>* _tokens, int64_t& _stepper);
	bool IsArrayIdentifier(std::vector<Token>* _tokens, int64_t& _stepper);
	void LexCompound(std::vector<Token>* _tokens, int64_t& _stepper);

	bool IsBreakChar(const char _char) {
		return _char == ':' || _char == ',';
	}

	bool IsDigit(const char _char) {
		return _char == '0' || _char == '1' || _char == '2' || _char == '3' || _char == '4' || _char == '5' ||
		       _char == '6' || _char == '7' || _char == '8' || _char == '9' || _char == '.';
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
	// Returns true if it skipped characters
	bool SkipWhitespace() {
		int64_t savedStep = stepper;
		while (std::isspace(nbtString[stepper])) {
			stepper++;
		}
		return savedStep != stepper;
	}
};