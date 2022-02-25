// Copyright 2022 Justus Zorn

#include <iostream>

#include "Net.h"
#include "Scene.h"

using namespace Hazard;

constexpr std::uint16_t HAZARD_DEFAULT_PORT = 34344;

Scene::Scene(std::string script, std::uint16_t port) : path{ script } {
	L = luaL_newstate();
	if (!L) {
		std::cerr << "ERROR: Lua initialization failed\n";
		return;
	}

	luaL_openlibs(L);

	Reload();

	ENetAddress address = { 0 };
	address.host = ENET_HOST_ANY;
	if (port == 0) {
		address.port = HAZARD_DEFAULT_PORT;
	}
	else {
		address.port = port;
	}

	host = enet_host_create(&address, 32, 2, 0, 0);
	if (!host) {
		std::cerr << "ERROR: Could not create ENet host\n";
		return;
	}
}

Scene::~Scene() {
	lua_close(L);
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

				std::cout << "INFO: " << player->player_name << " left the game\n";
				players.erase(player->player_name);
			}
			break;
		case ENET_EVENT_TYPE_RECEIVE:
			if (event.channelID == 0) {
				ReadPacket packet(event.packet->data, event.packet->dataLength);
				std::string player_name = packet.ReadString();

				// TODO: Call OnLogin function in Lua script
				std::cout << "INFO: " << player_name << " joined the game\n";

				Player& player = players[player_name];
				player.player_name = player_name;
				player.peer = event.peer;

				event.peer->data = &player;
			}
			break;
		}
	}
}

void Scene::Reload() {
	if (luaL_dofile(L, path.c_str()) != LUA_OK) {
		std::cerr << "ERROR: Could not load Lua script: " << lua_tostring(L, -1) << '\n';
	}
}
