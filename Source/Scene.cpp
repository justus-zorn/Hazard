// Copyright 2022 Justus Zorn

#include <iostream>

#include <SDL.h>

#include "Net.h"
#include "Scene.h"

using namespace Hazard;

Scene::Scene(std::string script, Config& config, std::uint16_t port) : config{ config }, script(script, this) {
	ENetAddress address = { 0 };
	address.host = ENET_HOST_ANY;
	if (port == 0) {
		address.port = config.Port();
	}
	else {
		address.port = port;
	}

	host = enet_host_create(&address, config.MaxPlayers(), 3, 0, 0);
	if (!host) {
		std::cerr << "ERROR: Could not create ENet host\n";
		return;
	}

	lastTicks = SDL_GetTicks64();

	std::uint32_t i = 0;
	for (const std::string& texture : config.GetTextures()) {
		loadedTextures[texture] = i++;
	}
}

Scene::~Scene() {
	for (auto& player : players) {
		enet_peer_disconnect_now(player.second.peer, 0);
	}
	enet_host_destroy(host);
}

static const char* GetButtonName(std::uint8_t button) {
	switch (button) {
	case SDL_BUTTON_LEFT:
		return "Left";
	case SDL_BUTTON_MIDDLE:
		return "Middle";
	case SDL_BUTTON_RIGHT:
		return "Right";
	case SDL_BUTTON_X1:
		return "X1";
	case SDL_BUTTON_X2:
		return "X2";
	default:
		std::cerr << "ERROR: Invalid button " << button << '\n';
		return "";
	}
}

void Scene::Update() {
	ENetEvent event;
	while (enet_host_service(host, &event, 0) > 0) {
		switch (event.type) {
		case ENET_EVENT_TYPE_CONNECT:
			event.peer->data = nullptr;
			break;
		case ENET_EVENT_TYPE_DISCONNECT:
		case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
			if (event.peer->data != nullptr) {
				Player* player = reinterpret_cast<Player*>(event.peer->data);
				script.OnDisconnect(player->playerName);
				players.erase(player->playerName);
			}
			break;
		case ENET_EVENT_TYPE_RECEIVE:
			if (event.channelID == 0) {
				ReadPacket packet(event.packet);
				std::string playerName = packet.ReadString();

				if (players.find(playerName) != players.end() || !script.OnPreLogin(playerName)) {
					enet_peer_disconnect(event.peer, 0);
				}
				else {
					Player& player = players[playerName];
					player.playerName = playerName;
					player.peer = event.peer;

					event.peer->data = &player;

					script.OnPostLogin(playerName);
				}
			}
			else if (event.channelID == 2) {
				if (event.peer->data == nullptr) {
					break;
				}

				Player* player = reinterpret_cast<Player*>(event.peer->data);

				ReadPacket packet(event.packet);
				std::uint32_t keyboardInputs = packet.Read32();
				for (std::uint32_t i = 0; i < keyboardInputs; ++i) {
					std::string key = SDL_GetKeyName(packet.Read32());
					if (packet.Read8()) {
						players[player->playerName].keys[key] = true;
						script.OnKeyEvent(player->playerName, key, true);
					}
					else {
						players[player->playerName].keys[key] = false;
						script.OnKeyEvent(player->playerName, key, false);
					}
				}
				std::uint32_t buttonInputs = packet.Read32();
				for (std::uint32_t i = 0; i < buttonInputs; ++i) {
					std::string button = GetButtonName(packet.Read8());
					if (packet.Read8()) {
						players[player->playerName].buttons[button] = true;
						script.OnButtonEvent(player->playerName, button, true);
					}
					else {
						players[player->playerName].buttons[button] = false;
						script.OnButtonEvent(player->playerName, button, false);
					}
				}
				std::int32_t mouseMotionX = packet.Read32();
				std::int32_t mouseMotionY = packet.Read32();
				if (packet.Read8()) {
					players[player->playerName].mouseX = mouseMotionX;
					players[player->playerName].mouseY = mouseMotionY;
					script.OnAxisEvent(player->playerName, "Mouse X", mouseMotionX);
					script.OnAxisEvent(player->playerName, "Mouse Y", mouseMotionY);
				}
			}
			enet_packet_destroy(event.packet);
			break;
		}
	}

	std::uint64_t now = SDL_GetTicks64();
	double dt = (now - lastTicks) / 1000.0;
	lastTicks = now;
	script.OnTick(dt);

	for (const std::string& kickedPlayer : kickedPlayers) {
		if (players.find(kickedPlayer) != players.end()) {
			enet_peer_disconnect(players[kickedPlayer].peer, 0);
		}
	}

	kickedPlayers.clear();

	for (auto& pair : players) {
		WritePacket statePacket;
		statePacket.Write32(static_cast<std::uint32_t>(pair.second.sprites.size()));
		for (const Sprite& sprite : pair.second.sprites) {
			statePacket.Write32(sprite.x);
			statePacket.Write32(sprite.y);
			statePacket.Write32(sprite.scale);
			statePacket.Write32(sprite.texture);
			statePacket.Write32(sprite.animation);
		}
		enet_peer_send(pair.second.peer, 1, statePacket.GetPacket(false));
		pair.second.sprites.clear();
	}
}

void Scene::Reload() {
	config.Reload();

	loadedTextures.clear();
	std::uint32_t i = 0;
	for (const std::string& texture : config.GetTextures()) {
		loadedTextures[texture] = i++;
	}

	script.Reload();
}

std::vector<std::string> Scene::GetPlayers() {
	std::vector<std::string> list;
	for (const auto& pair : players) {
		list.push_back(pair.first);
	}
	return list;
}

bool Scene::IsOnline(const std::string& playerName) {
	return players.find(playerName) != players.end();
}

void Scene::Kick(const std::string& playerName) {
	kickedPlayers.push_back(playerName);
}

bool Scene::KeyDown(const std::string& playerName, const std::string& key) {
	return players[playerName].keys[key];
}

bool Scene::ButtonDown(const std::string& playerName, const std::string& button) {
	return players[playerName].buttons[button];
}

std::int32_t Scene::GetAxis(const std::string& playerName, const std::string& axis) {
	if (axis == "Mouse X") {
		return players[playerName].mouseX;
	}
	else if (axis == "Mouse Y") {
		return players[playerName].mouseY;
	}
	else {
		return 0;
	}
}

bool Scene::IsTextureLoaded(const std::string& texture) {
	return loadedTextures.find(texture) != loadedTextures.end();
}

void Scene::DrawSprite(const std::string& playerName, const std::string& texture, std::int32_t x, std::int32_t y, std::uint32_t scale, std::uint32_t animation) {
	players[playerName].sprites.push_back({ x, y, scale, loadedTextures[texture], animation });
}
