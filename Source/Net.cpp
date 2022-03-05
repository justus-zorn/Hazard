// Copyright 2022 Justus Zorn

#include <cstring>
#include <iostream>

#include <SDL.h>

#include "Net.h"

using namespace Hazard;

void WritePacket::Write8(std::uint8_t value) {
	data.push_back(value);
}

void WritePacket::Write32(std::uint32_t value) {
	std::uint32_t start = static_cast<std::uint32_t>(data.size());
	data.resize(start + 4);
	*reinterpret_cast<std::uint32_t*>(&data[start]) = SDL_SwapBE32(value);
}

void WritePacket::WriteString(const std::string& value) {
	Write32(static_cast<std::uint32_t>(value.length()));
	if (value.length() > 0) {
		std::uint32_t start = static_cast<std::uint32_t>(data.size());
		data.resize(start + value.length());
		std::memcpy(&data[start], value.data(), value.length());
	}
}

ENetPacket* WritePacket::GetPacket(bool reliable) {
	return enet_packet_create(data.data(), data.size(), reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
}

ReadPacket::ReadPacket(ENetPacket* packet) : data{ packet->data }, dataLength{ static_cast<std::uint32_t>(packet->dataLength) } {}

std::uint8_t ReadPacket::Read8() {
	if (index < dataLength) {
		return data[index++];
	}
	else {
		std::cerr << "ERROR: Detected invalid packet\n";
		return 0;
	}
}

std::uint32_t ReadPacket::Read32() {
	if (sizeof(std::uint32_t) <= dataLength && index <= dataLength - sizeof(std::uint32_t)) {
		std::uint32_t value = SDL_SwapBE32(*reinterpret_cast<const std::uint32_t*>(data + index));
		index += sizeof(std::uint32_t);
		return value;
	}
	else {
		std::cerr << "ERROR: Detected invalid packet\n";
		return 0;
	}
}

std::string ReadPacket::ReadString() {
	std::uint32_t stringLength = Read32();
	if (stringLength > 0) {
		if (stringLength <= dataLength && index <= dataLength - stringLength) {
			std::string value;
			value.resize(stringLength);
			std::memcpy(&value[0], data + index, stringLength);
			index += stringLength;
			return value;
		}
		else {
			std::cerr << "ERROR: Detected invalid packet\n";
		}
	}
	return "";
}
