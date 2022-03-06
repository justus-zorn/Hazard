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

	host = enet_host_create(&address, config.MaxPlayers(), 4, 0, 0);
	if (!host) {
		std::cerr << "ERROR: Could not create ENet host\n";
		return;
	}

	lastTicks = SDL_GetTicks64();

	std::uint32_t i = 0;
	for (const std::string& texture : config.GetTextures()) {
		loadedTextures[texture] = i++;
	}

	i = 0;
	for (const std::string& sound : config.GetSounds()) {
		loadedSounds[sound] = i++;
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

				if (players.find(playerName) != players.end() || !script.OnLogin(playerName)) {
					enet_peer_disconnect(event.peer, 0);
				}
				else {
					Player& player = players[playerName];
					player.playerName = playerName;
					player.peer = event.peer;

					event.peer->data = &player;

					script.OnJoin(playerName);
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
					SDL_Keycode keycode = packet.Read32();
					std::uint8_t pressed = packet.Read8();
					std::string key = SDL_GetKeyName(keycode);
					if (pressed) {
						players[player->playerName].keys[key] = true;
						script.OnKeyEvent(player->playerName, key, true);
					}
					else {
						players[player->playerName].keys[key] = false;
						script.OnKeyEvent(player->playerName, key, false);
					}
					if (keycode == SDLK_BACKSPACE && pressed) {
						std::string& composition = players[player->playerName].composition;
						while (composition.length() > 0 && (composition[composition.length() - 1] & 0xC0) == 0x80) {
							composition.erase(composition.end() - 1);
						}
						if (composition.length() > 0) {
							composition.erase(composition.end() - 1);
						}
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
				players[player->playerName].composition += packet.ReadString();
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
			if (sprite.isText) {
				statePacket.Write8(1);
				statePacket.Write8(sprite.r);
				statePacket.Write8(sprite.g);
				statePacket.Write8(sprite.b);
				statePacket.WriteString(sprite.text);
			}
			else {
				statePacket.Write8(0);
				statePacket.Write32(sprite.texture);
				statePacket.Write32(sprite.animation);
			}
		}
		enet_peer_send(pair.second.peer, 1, statePacket.GetPacket(false));
		pair.second.sprites.clear();

		WritePacket audioPacket;
		audioPacket.Write32(static_cast<std::uint32_t>(pair.second.audioCommands.size()));
		for (const AudioCommand& audioCommand : pair.second.audioCommands) {
			audioPacket.Write8(static_cast<std::uint8_t>(audioCommand.type));
			audioPacket.Write8(audioCommand.volume);
			audioPacket.Write16(audioCommand.channel);
			audioPacket.Write32(audioCommand.sound);
		}
		enet_peer_send(pair.second.peer, 3, audioPacket.GetPacket(true));
		pair.second.audioCommands.clear();
	}
}

void Scene::Reload() {
	config.Reload();

	loadedTextures.clear();
	std::uint32_t i = 0;
	for (const std::string& texture : config.GetTextures()) {
		loadedTextures[texture] = i++;
	}

	loadedSounds.clear();
	i = 0;
	for (const std::string& sound : config.GetSounds()) {
		loadedSounds[sound] = i++;
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

bool Scene::IsKeyDown(const std::string& playerName, const std::string& key) {
	return players[playerName].keys[key];
}

bool Scene::IsButtonDown(const std::string& playerName, const std::string& button) {
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

const std::string& Scene::GetComposition(const std::string& playerName) {
	return players[playerName].composition;
}

void Scene::SetComposition(const std::string& playerName, std::string composition) {
	players[playerName].composition = composition;
}

bool Scene::IsTextureLoaded(const std::string& texture) {
	return loadedTextures.find(texture) != loadedTextures.end();
}

void Scene::DrawSprite(const std::string& playerName, const std::string& texture, std::int32_t x, std::int32_t y, std::uint32_t scale, std::uint32_t animation) {
	Sprite sprite;
	sprite.isText = false;
	sprite.x = x;
	sprite.y = y;
	sprite.scale = scale;
	sprite.texture = loadedTextures[texture];
	sprite.animation = animation;

	players[playerName].sprites.push_back(sprite);
}

void Scene::DrawTextSprite(const std::string& playerName, const std::string& text, std::int32_t x, std::int32_t y, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint32_t lineLength) {
	Sprite sprite;
	sprite.isText = true;
	sprite.x = x;
	sprite.y = y;
	sprite.scale = lineLength;
	sprite.text = text;
	sprite.r = r;
	sprite.g = g;
	sprite.b = b;

	players[playerName].sprites.push_back(sprite);
}

bool Scene::IsSoundLoaded(const std::string& sound) {
	return loadedSounds.find(sound) != loadedSounds.end();
}

bool Scene::IsChannelValid(std::uint16_t channel) {
	return channel < config.Channels();
}

void Scene::Play(const std::string& playerName, const std::string& sound, std::uint8_t volume, std::uint16_t channel) {
	AudioCommand audioCommand;
	audioCommand.type = AudioCommand::Type::Play;
	audioCommand.volume = volume;
	audioCommand.channel = channel;
	audioCommand.sound = loadedSounds[sound];

	players[playerName].audioCommands.push_back(audioCommand);
}

void Scene::PlayAny(const std::string& playerName, const std::string& sound, std::uint8_t volume) {
	AudioCommand audioCommand;
	audioCommand.type = AudioCommand::Type::PlayAny;
	audioCommand.volume = volume;
	audioCommand.channel = 0;
	audioCommand.sound = loadedSounds[sound];

	players[playerName].audioCommands.push_back(audioCommand);
}

void Scene::Stop(const std::string& playerName, std::uint16_t channel) {
	AudioCommand audioCommand;
	audioCommand.type = AudioCommand::Type::Stop;
	audioCommand.volume = 0;
	audioCommand.channel = channel;
	audioCommand.sound = 0;

	players[playerName].audioCommands.push_back(audioCommand);
}

void Scene::StopAll(const std::string& playerName) {
	AudioCommand audioCommand;
	audioCommand.type = AudioCommand::Type::StopAll;
	audioCommand.volume = 0;
	audioCommand.channel = 0;
	audioCommand.sound = 0;

	players[playerName].audioCommands.push_back(audioCommand);
}
