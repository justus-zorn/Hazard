// Copyright 2022 Justus Zorn

#include <iostream>

#include "Client.h"
#include "Net.h"

using namespace Hazard;

Client::Client(const std::string& playerName, const std::string& address, std::uint16_t defaultPort) {
	std::string hostname;
	std::uint16_t port;
	if (address.find(':') < address.size()) {
		hostname = address.substr(0, address.find(':'));
		port = std::stoi(address.substr(address.find(':') + 1));
	}
	else {
		hostname = address;
		port = defaultPort;
	}

	host = enet_host_create(nullptr, 1, 3, 0, 0);
	if (!host) {
		std::cerr << "ERROR: Could not create ENet host\n";
		return;
	}

	ENetAddress serverAddress = { 0 };
	serverAddress.port = port;
	if (enet_address_set_host(&serverAddress, hostname.c_str()) < 0) {
		std::cerr << "ERROR: Could not resolve " << hostname << '\n';
		return;
	}

	server = enet_host_connect(host, &serverAddress, 3, 0);
	if (!server) {
		std::cerr << "ERROR: Could not connect to " << address << '\n';
		return;
	}

	ENetEvent event;
	if (enet_host_service(host, &event, 3000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
		WritePacket packet;
		packet.WriteString(playerName);

		enet_peer_send(server, 0, packet.GetPacket(true));
	}
	else {
		std::cerr << "ERROR: Could not connect to " << address << '\n';
	}
}

Client::~Client() {
	enet_peer_disconnect_now(server, 0);
	enet_host_destroy(host);
}

bool Client::Update(const Input& input) {
	ENetEvent event;
	while (enet_host_service(host, &event, 0) > 0) {
		switch (event.type) {
		case ENET_EVENT_TYPE_DISCONNECT:
		case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
			return false;
		case ENET_EVENT_TYPE_RECEIVE:
			if (event.channelID == 1) {
				ReadPacket packet(event.packet);
				std::uint32_t spriteCount = packet.Read32();
				sprites.resize(spriteCount);
				for (std::uint32_t i = 0; i < spriteCount; ++i) {
					Sprite& sprite = sprites[i];

					sprite.x = packet.Read32();
					sprite.y = packet.Read32();
					sprite.scale = packet.Read32();

					if (packet.Read8()) {
						// Text
						sprite.isText = true;
						sprite.r = packet.Read8();
						sprite.g = packet.Read8();
						sprite.b = packet.Read8();
						sprite.text = packet.ReadString();
					}
					else {
						// Sprite
						sprite.isText = false;
						sprite.texture = packet.Read32();
						sprite.animation = packet.Read32();
					}
				}
			}
			enet_packet_destroy(event.packet);
			break;
		}
	}

	WritePacket inputPacket;
	inputPacket.Write32(static_cast<std::uint32_t>(input.keyboardInputs.size()));
	for (KeyboardInput keyboardInput : input.keyboardInputs) {
		inputPacket.Write32(keyboardInput.key);
		inputPacket.Write8(keyboardInput.pressed);
	}
	inputPacket.Write32(static_cast<std::uint32_t>(input.buttonInputs.size()));
	for (ButtonInput buttonInput : input.buttonInputs) {
		inputPacket.Write8(buttonInput.button);
		inputPacket.Write8(buttonInput.pressed);
	}
	inputPacket.Write32(input.mouseMotionX);
	inputPacket.Write32(input.mouseMotionY);
	inputPacket.Write8(input.mouseMotion);

	enet_peer_send(server, 2, inputPacket.GetPacket(true));

	return true;
}

const std::vector<Sprite>& Client::GetSprites() const {
	return sprites;
}
