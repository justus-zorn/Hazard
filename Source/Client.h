// Copyright 2022 Justus Zorn

#ifndef Hazard_Client_h
#define Hazard_Client_h

#include <cstdint>
#include <string>

#include <enet.h>

namespace Hazard {
	class Client {
	public:
		Client(const std::string& address, std::uint16_t defaultPort);
		Client(const Client&) = delete;
		~Client();

		Client& operator=(const Client&) = delete;

		bool Update();

	private:
		ENetHost* host = nullptr;
		ENetPeer* server = nullptr;
	};
}

#endif
