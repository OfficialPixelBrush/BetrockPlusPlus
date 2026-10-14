/*
 * Copyright (c) 2026, Anya Rihtarshich <vesui@proton.me>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include "server.h"

#include <span>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <memory>
#include <string_view>
#include <utility>

#include <replxx.hxx>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <poll.h>
#include <unistd.h>
#endif

struct ServerConsole::Impl {
	replxx::Replxx editor;
};

ServerConsole::ServerConsole() = default;

// entire class needs serious review, thank you -AR
namespace {
bool HasInteractiveTerminal() {
#if defined(_WIN32)
	DWORD inputMode = 0;
	DWORD outputMode = 0;

	if (!GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &inputMode) ||
	    !GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &outputMode))
		return false;
	return true;
#else
	if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO))
		return false;
	const char* term = std::getenv("TERM");
	return term && *term && std::string_view(term) != "dumb";
#endif
}

bool IsWhitespaceOnly(std::string_view line) {
	return std::all_of(line.begin(), line.end(), [](unsigned char c) { return std::isspace(c) != 0; });
}

int CodePointCount(std::string_view text) {
	int count = 0;
	for (unsigned char c : text)
		if ((c & 0xC0) != 0x80)
			++count;
	return count;
}
}

ServerConsole::~ServerConsole() {
	Stop();
}

void ServerConsole::Start(const CommandManager& _commands) {
	if (reader.joinable()) return;
	commands = &_commands;
	stopping.store(false);

	interactive = HasInteractiveTerminal();
	if (interactive) {
		impl = std::make_unique<Impl>();
		impl->editor.bind_key(replxx::Replxx::KEY::TAB, [this](char32_t) {
			return impl->editor.invoke(replxx::Replxx::ACTION::COMPLETE_LINE, 0);
		});

		auto cancel = [this](char32_t) {
			stopping.store(true);
			shutdownRequested.store(true);
			return replxx::Replxx::ACTION_RESULT::BAIL;
		};

		impl->editor.bind_key(replxx::Replxx::KEY::control('C'), cancel);
		impl->editor.bind_key(replxx::Replxx::KEY::meta(replxx::Replxx::KEY::control('C')), cancel);
		impl->editor.set_completion_callback([this](const std::string& input, int& contextLen) {
			return Complete(input, contextLen);
		});

		GlobalLogger().SetTerminalSink([this](const char* data, int length) {
			if (impl)
				impl->editor.write(data, length);
		});
	}
	readerDone.store(false);
	reader = std::thread(&ServerConsole::ReaderLoop, this);
}

void ServerConsole::Stop() {
	if (!reader.joinable()) {
		GlobalLogger().SetTerminalSink({});
		return;
	}
	stopping.store(true);
	if (interactive && impl)
		impl->editor.emulate_key_press(replxx::Replxx::KEY::control('C'));
#if defined(_WIN32)
	else {
		while (!readerDone.load()) {
			CancelSynchronousIo(reader.native_handle());
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}
#endif
	reader.join();
	GlobalLogger().SetTerminalSink({});
	impl.reset();
	interactive = false;
}

void ServerConsole::DrainCommands(std::vector<std::string>& _commands) {
	std::lock_guard lock(queueMutex);
	while (!commandQueue.empty()) {
		_commands.push_back(std::move(commandQueue.front()));
		commandQueue.pop_front();
	}
}

void ServerConsole::UpdatePlayerNames(const std::vector<std::string>& _names) {
	std::lock_guard lock(namesMutex);
	if (playerNames != _names)
		playerNames = _names;
}

replxx::Replxx::completions_t ServerConsole::Complete(std::string_view input, int& contextLen) const {
	std::vector<strategos::Suggestion> suggestions;
	{
		std::lock_guard lock(namesMutex);
		suggestions = commands->Suggest(input, true, std::span<const std::string>(playerNames));
	}
	size_t start = input.size();
	for (const auto& suggestion : suggestions)
		start = std::min(start, suggestion.start);
	contextLen = CodePointCount(input.substr(start));
	std::vector<replxx::Replxx::Completion> completions;
	completions.reserve(suggestions.size());
	for (const auto& suggestion : suggestions)
		completions.emplace_back(suggestion.text);
	return completions;
}

void ServerConsole::QueueLine(std::string line) {
	if (!line.empty() && line.back() == '\r')
		line.pop_back();
	if (IsWhitespaceOnly(line) || stopping.load())
		return;
	std::lock_guard lock(queueMutex);
	if (!stopping.load())
		commandQueue.push_back(std::move(line));
}


void ServerConsole::ReaderLoop() {
	if (interactive) {
		while (!stopping.load()) {
			const char* line = impl->editor.input("> ");
			if (!line)
				break;
			QueueLine(line);
		}
		readerDone.store(true);
		return;
	}
#if defined(_WIN32)
	HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
	std::string line;
	char buffer[256];
	while (!stopping.load()) {
		DWORD count = 0;
		if (!ReadFile(input, buffer, sizeof(buffer), &count, nullptr) || count == 0)
			break;
		for (DWORD i = 0; i < count; ++i) {
			if (buffer[i] == '\n') {
				QueueLine(std::move(line));
				line.clear();
			} else {
				line.push_back(buffer[i]);
			}
		}
	}
	if (!line.empty())
		QueueLine(std::move(line));
#else
	std::string line;
	char buffer[256];
	while (!stopping.load()) {
		pollfd descriptor{ STDIN_FILENO, POLLIN, 0 };
		int status = poll(&descriptor, 1, 50);
		if (status <= 0)
			continue;
		ssize_t count = read(STDIN_FILENO, buffer, sizeof(buffer));
		if (count <= 0)
			break;
		for (ssize_t i = 0; i < count; ++i) {
			if (buffer[i] == '\n') {
				QueueLine(std::move(line));
				line.clear();
			} else {
				line.push_back(buffer[i]);
			}
		}
	}
	if (!line.empty())
		QueueLine(std::move(line));
#endif
	readerDone.store(true);
}
