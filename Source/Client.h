// Copyright 2022 Justus Zorn

#ifndef Hazard_Client_h
#define Hazard_Client_h

#include <cstdint>
#include <string>
#include <vector>

#include <enet.h>

#include "Common.h"

namespace Hazard {
	class Client {
	public:
		Client(const std::string& playerName, const std::string& address, std::uint16_t defaultPort);
		Client(const Client&) = delete;
		~Client();

		Client& operator=(const Client&) = delete;

		bool Update(const Input& input);

		const std::vector<Sprite>& GetSprites() const;
		const std::vector<AudioCommand>& GetAudioCommands() const;

	private:
		ENetHost* host = nullptr;
		ENetPeer* server = nullptr;

		std::vector<Sprite> sprites;
		std::vector<AudioCommand> audioCommands;
	};
}

#endif
