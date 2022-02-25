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
				Lua_OnDisconnect(player->playerName);
				players.erase(player->playerName);
			}
			break;
		case ENET_EVENT_TYPE_RECEIVE:
			if (event.channelID == 0) {
				ReadPacket packet(event.packet);
				std::string playerName = packet.ReadString();

				if (players.find(playerName) != players.end() || !Lua_OnPreLogin(playerName)) {
					enet_peer_disconnect(event.peer, 0);
				}
				else {
					Player& player = players[playerName];
					player.playerName = playerName;
					player.peer = event.peer;

					event.peer->data = &player;

					Lua_OnPostLogin(playerName);
				}
			}
			enet_packet_destroy(event.packet);
			break;
		}
	}

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
	}
}

void Scene::Reload() {
	lua_newtable(L);
	lua_setglobal(L, "Game");

	if (luaL_dofile(L, path.c_str()) != LUA_OK) {
		std::cerr << "ERROR: Error while loading Lua script: " << lua_tostring(L, -1) << '\n';
	}
}

bool Scene::Lua_OnPreLogin(const std::string& playerName) {
	lua_getglobal(L, "Game");
	lua_getfield(L, -1, "OnPreLogin");

	if (lua_isnil(L, -1)) {
		lua_settop(L, 0);
		return true;
	}

	lua_pushstring(L, playerName.c_str());

	if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
		std::cerr << "ERROR: Error while calling Game.OnPreLogin: " << lua_tostring(L, -1) << '\n';
		lua_settop(L, 0);
		return false;
	}

	if (!lua_isboolean(L, -1)) {
		std::cerr << "ERROR: Game.OnPreLogin must return a boolean\n";
		lua_settop(L, 0);
		return false;
	}

	bool result = lua_toboolean(L, -1);
	lua_settop(L, 0);
	return result;
}

void Scene::Lua_OnPostLogin(const std::string& playerName) {
	lua_getglobal(L, "Game");
	lua_getfield(L, -1, "OnPostLogin");

	if (lua_isnil(L, -1)) {
		lua_settop(L, 0);
		return;
	}

	lua_pushstring(L, playerName.c_str());

	if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
		std::cerr << "ERROR: Error while calling Game.OnPostLogin: " << lua_tostring(L, -1) << '\n';
	}

	lua_settop(L, 0);
}

void Scene::Lua_OnDisconnect(const std::string& playerName) {
	lua_getglobal(L, "Game");
	lua_getfield(L, -1, "OnDisconnect");

	if (lua_isnil(L, -1)) {
		lua_settop(L, 0);
		return;
	}

	lua_pushstring(L, playerName.c_str());

	if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
		std::cerr << "ERROR: Error while calling Game.OnDisconnect: " << lua_tostring(L, -1) << '\n';
	}

	lua_settop(L, 0);
}
