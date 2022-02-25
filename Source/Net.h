// Copyright 2022 Justus Zorn

#ifndef Hazard_Net_h
#define Hazard_Net_h

#include <cstdint>
#include <string>
#include <vector>

namespace Hazard {
	class WritePacket {
	public:
		void Write32(std::uint32_t value);
		void WriteString(const std::string& value);

		const std::uint8_t* Data() const;
		std::uint32_t Length() const;

	private:
		std::vector<std::uint8_t> data;
	};

	class ReadPacket {
	public:
		ReadPacket(const std::uint8_t* data, std::uint32_t length);

		std::uint32_t Read32();
		std::string ReadString();

	private:
		const std::uint8_t* data;
		std::uint32_t index = 0;
		std::uint32_t dataLength;
	};
}

#endif
