// Copyright 2022 Justus Zorn

#include <cstring>
#include <iostream>

#include <SDL.h>

#include "Net.h"

using namespace Hazard;

void WritePacket::Write32(std::uint32_t value) {
	std::uint32_t start = static_cast<std::uint32_t>(data.size());
	data.resize(start + 4);
	*reinterpret_cast<std::uint32_t*>(&data[start]) = SDL_SwapBE32(value);
}

void WritePacket::WriteString(const std::string& value) {
	Write32(static_cast<std::uint32_t>(value.length()));
	std::uint32_t start = static_cast<std::uint32_t>(data.size());
	data.resize(start + value.size());
	std::memcpy(&data[start], value.data(), value.length());
}

const std::uint8_t* WritePacket::Data() const {
	return data.data();
}

std::uint32_t WritePacket::Length() const {
	return static_cast<std::uint32_t>(data.size());
}

ReadPacket::ReadPacket(const std::uint8_t* data, std::uint32_t length) : data{ data }, dataLength{ length } {}

std::uint32_t ReadPacket::Read32() {
	if (sizeof(std::uint32_t) < dataLength && index <= dataLength - sizeof(std::uint32_t)) {
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
	if (stringLength < dataLength && index <= dataLength - stringLength) {
		std::string value;
		value.resize(stringLength);
		std::memcpy(&value[0], data + index, stringLength);
		index += stringLength;
		return value;
	}
	else {
		std::cerr << "ERROR: Detected invalid packet\n";
		return "";
	}
}