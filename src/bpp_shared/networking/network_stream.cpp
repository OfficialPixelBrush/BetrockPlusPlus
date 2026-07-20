/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "network_stream.h"
#include "logger.h"
#include "numeric_structs.h"
#include "packet_data.h"
#include "ucs2.h"
#include <string>
#include <vector>

NetworkStream::NetworkStream(int p_client_socket) {
	client_socket = p_client_socket;
}

NetworkStream::~NetworkStream() {
	if (client_socket != INVALID_SOCKET) {
#if defined(_WIN32) || defined(_WIN64)
		shutdown(client_socket, SD_BOTH);
		closesocket(client_socket);
		// TODO: Clean-up WSA when the server closes
		// WSACleanup();
#else
		shutdown(client_socket, SHUT_RDWR);
		close(client_socket);
#endif
		client_socket = INVALID_SOCKET;
	}
}

void NetworkStream::flushWriteBufferBlocking() {
	if (writeBuffer.empty() || client_socket == INVALID_SOCKET)
		return;

	// Switch to blocking mode
#if defined(_WIN32) || defined(_WIN64)
	u_long mode = 0;
	ioctlsocket(client_socket, FIONBIO, &mode);
#else
	int flags = fcntl(client_socket, F_GETFL, 0);
	fcntl(client_socket, F_SETFL, flags & ~O_NONBLOCK);
#endif

	size_t sent = 0;
	while (sent < writeBuffer.size()) {
		int result = send(client_socket, reinterpret_cast<const char*>(writeBuffer.data() + sent),
		                  static_cast<int>(writeBuffer.size() - sent), 0);
		if (result <= 0)
			break;
		sent += static_cast<size_t>(result);
	}
	writeBuffer.clear();

	// We close here so the client can get the packet data we just sent out before we disconnect
#if defined(_WIN32) || defined(_WIN64)
	shutdown(client_socket, SD_SEND);
	closesocket(client_socket);
#else
	shutdown(client_socket, SHUT_WR);
	close(client_socket);
#endif
	client_socket = INVALID_SOCKET;
}

// String-8 Handling
void NetworkStream::WriteString8(const std::string& str) {
	uint16_t length = static_cast<uint16_t>(str.size());
	Write(length);
	std::vector<uint8_t> data;
	data.reserve(str.size() * 2);
	for (const char c : str) {
		data.push_back(static_cast<uint8_t>(c));
	}
	WriteBytes(data.data(), data.size());
}

std::string NetworkStream::ReadString8() {
	uint16_t len = Read<uint16_t>(); // adjust to uint32_t if your protocol uses 4-byte lengths
	std::string result(len, '\0');
	ReadBytes(reinterpret_cast<uint8_t*>(result.data()), len);
	return result;
}

// String-16 Handling
void NetworkStream::WriteString16(const std::string& str) {
	std::u16string str16 = ToUCS2(str);
	uint16_t length = static_cast<uint16_t>(str16.size());
	Write(length);
	std::vector<uint8_t> data;
	data.reserve(str16.size());
	for (const char16_t c : str16) {
		data.push_back(static_cast<uint8_t>((c >> 8) & 0xFF));
		data.push_back(static_cast<uint8_t>(c & 0xFF));
	}
	WriteBytes(data.data(), data.size());
}

std::string NetworkStream::ReadString16() {
	uint16_t len = Read<uint16_t>();

	// Read as UTF-16 (2 bytes per char) regardless of platform wchar_t size
	std::vector<uint16_t> buf(len);
	ReadBytes(reinterpret_cast<uint8_t*>(buf.data()), len * sizeof(char16_t));

	std::u16string result(len, u'\0');
	for (uint16_t i = 0; i < len; i++) {
		// byteswap each UTF-16 unit, then widen to wchar_t
		result[i] = static_cast<char16_t>(__builtin_bswap16(buf[i]));
	}
	return ToUTF8(result);
}

size_t NetworkStream::ReadBytes(uint8_t* buf, size_t len) {
	size_t received = 0;

	// 1. consume existing buffered data
	while (!readBackBuffer.empty() && received < len) {
		buf[received++] = readBackBuffer.front();
		readBackBuffer.pop_front();
	}

	// 2. try recv until we either fill or would block
	while (received < len) {
		int result = recv(client_socket, reinterpret_cast<char*>(buf + received), static_cast<int>(len - received), 0);

		if (result > 0) {
			received += result;
		} else if (result == 0) {
			connected = false;
			return received;
		} else {
#ifdef _WIN32
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK) {
				break;
			}
#else
			if (errno == EWOULDBLOCK || errno == EAGAIN) {
				break;
			}
#endif
			connected = false;
			return received;
		}
	}

	return received;
}

void NetworkStream::WriteBytes(const uint8_t* buf, size_t len) {
	// Append to the write buffer -- no syscall here.
	// The actual send() happens once per tick in flushWriteBuffer().
	writeBuffer.insert(writeBuffer.end(), buf, buf + len);
}

// TODO: Due to how this system works, a concrete length is never supplied.
// Data is read until 0x7F is hit. Ideally we should exit out if we're past
// a certain number of bytes
void NetworkStream::ReadEntityMetadata(std::vector<PacketData::EntityMetadata::DataEntry>& metadata) {
	uint8_t val = Read<uint8_t>();
	while (val != PacketData::EntityMetadata::END) {
		// What type the data has
		PacketData::EntityMetadata::Type type = PacketData::EntityMetadata::Type(val >> 5);
		// Where the data goes for the relevant entity
		uint8_t index = uint8_t(val & 0x1F);
		switch (type) {
		case PacketData::EntityMetadata::Type::BYTE: {
			int8_t num = Read<int8_t>();
			metadata.push_back(PacketData::EntityMetadata::DataEntry{ .type = type, .index = index, .value = num });
			break;
		}
		case PacketData::EntityMetadata::Type::SHORT: {
			int16_t num = Read<int16_t>();
			metadata.push_back(PacketData::EntityMetadata::DataEntry{ .type = type, .index = index, .value = num });
			break;
		}
		case PacketData::EntityMetadata::Type::INTEGER: {
			int32_t num = Read<int32_t>();
			metadata.push_back(PacketData::EntityMetadata::DataEntry{ .type = type, .index = index, .value = num });
			break;
		}
		case PacketData::EntityMetadata::Type::FLOAT: {
			float num = Read<float>();
			metadata.push_back(PacketData::EntityMetadata::DataEntry{ .type = type, .index = index, .value = num });
			break;
		}
		case PacketData::EntityMetadata::Type::STRING: {
			std::string str = ReadString16();
			metadata.push_back(PacketData::EntityMetadata::DataEntry{ .type = type, .index = index, .value = str });
			break;
		}
		case PacketData::EntityMetadata::Type::ITEM: {
			ItemStack item;
			item.id = Read<ItemId>();
			// TODO: Check if B1.7.3 actually does
			// this for Entity Metadata too
			if (item.id != Items::Id::INVALID) {
				item.count = Read<ItemAmount>();
				item.data = Read<ItemDamage>();
			}
			metadata.push_back(PacketData::EntityMetadata::DataEntry{ .type = type, .index = index, .value = item });
			break;
		}
		case PacketData::EntityMetadata::Type::COORDINATES: {
			Int32_3 coordinate(Read<int32_t>(), Read<int32_t>(), Read<int32_t>());
			metadata.push_back(
			    PacketData::EntityMetadata::DataEntry{ .type = type, .index = index, .value = coordinate });
			break;
		}
		default:
			GlobalLogger().warn << "NetworkStream::ReadEntityMetadata: Unknown metadata type "
			                    << static_cast<int>(type);
			break;
		}
		// Read in the next value
		val = Read<uint8_t>();
	}
}

// TODO: Implement this! Ideally we could just pass an entity into here
// and it'd take care of things automatically
void NetworkStream::WriteEntityMetadata(const std::vector<PacketData::EntityMetadata::DataEntry>& metadata) {
	for (auto& entry : metadata) {
		uint8_t val = (static_cast<uint8_t>(entry.type) << 5) | (entry.index & 0x1F);
		Write(val);
		switch (entry.type) {
		case PacketData::EntityMetadata::Type::BYTE:
			Write(std::get<int8_t>(entry.value));
			break;
		case PacketData::EntityMetadata::Type::SHORT:
			Write(std::get<int16_t>(entry.value));
			break;
		case PacketData::EntityMetadata::Type::INTEGER:
			Write(std::get<int32_t>(entry.value));
			break;
		case PacketData::EntityMetadata::Type::FLOAT:
			Write(std::get<float>(entry.value));
			break;
		case PacketData::EntityMetadata::Type::STRING:
			WriteString16(std::get<std::string>(entry.value));
			break;
		case PacketData::EntityMetadata::Type::ITEM:
			Write(std::get<ItemStack>(entry.value).id);
			if (std::get<ItemStack>(entry.value).id != Items::Id::INVALID) {
				Write(std::get<ItemStack>(entry.value).count);
				Write(std::get<ItemStack>(entry.value).data);
			}
			break;
		case PacketData::EntityMetadata::Type::COORDINATES:
			Write(std::get<Int32_3>(entry.value).x);
			Write(std::get<Int32_3>(entry.value).y);
			Write(std::get<Int32_3>(entry.value).z);
			break;
		default:
			GlobalLogger().warn << "NetworkStream::WriteEntityMetadata: Unknown metadata type "
			                    << static_cast<int>(entry.type);
			break;
		}
	}
	Write(PacketData::EntityMetadata::END);
}

bool NetworkStream::flushWriteBuffer() {
	if (writeBuffer.empty())
		return connected;
	size_t sent = 0;
	while (sent < writeBuffer.size()) {
		int result = send(client_socket, reinterpret_cast<const char*>(writeBuffer.data() + sent),
		                  static_cast<int>(writeBuffer.size() - sent), 0);
		if (result < 0) {
#if defined(_WIN32) || defined(_WIN64)
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
				break;
#else
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
#endif
			connected = false;
			break;
		}
		if (result == 0) {
			connected = false;
			break;
		}
		sent += static_cast<size_t>(result);
	}
	if (sent > 0) {
		writeBuffer.erase(writeBuffer.begin(),
		                  writeBuffer.begin() + static_cast<std::vector<unsigned char>::difference_type>(sent));
	}
	return connected;
}

bool NetworkStream::hasData() {
#if defined(_WIN32) || defined(_WIN64)
	// Check rollback buffer first, then the socket.
	if (!readBackBuffer.empty())
		return true;
	u_long bytesAvailable = 0;
	if (ioctlsocket(client_socket, FIONREAD, &bytesAvailable) == SOCKET_ERROR) {
		connected = false;
		return false;
	}
	return bytesAvailable > 0;
#else
	// Check rollback buffer first, then the socket.
	if (!readBackBuffer.empty())
		return true;
	int bytesAvailable = 0;
	if (ioctl(client_socket, FIONREAD, &bytesAvailable) < 0) {
		connected = false;
		return false;
	}
	return bytesAvailable > 0;
#endif
}