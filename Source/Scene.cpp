// Copyright 2022 Justus Zorn

#include <iostream>

#include <SDL.h>

#include "Net.h"
#include "Scene.h"

using namespace Hazard;

constexpr std::uint16_t HAZARD_DEFAULT_PORT = 34344;

Scene::Scene(std::string script, Config& config, std::uint16_t port) : config{ config }, path { script } {
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

	host = enet_host_create(&address, 32, 3, 0, 0);
	if (!host) {
		std::cerr << "ERROR: Could not create ENet host\n";
		return;
	}

	lastTick = SDL_GetTicks64();
}

Scene::~Scene() {
	lua_close(L);
}

static const char* GetMouseButtonName(std::uint8_t button) {
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
		std::cerr << "ERROR: Invalid mouse button " << button << '\n';
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
						Lua_OnKeyDown(player->playerName, key);
					}
					else {
						players[player->playerName].keys[key] = false;
						Lua_OnKeyUp(player->playerName, key);
					}
				}
				std::uint32_t mouseButtonInputs = packet.Read32();
				for (std::uint32_t i = 0; i < mouseButtonInputs; ++i) {
					std::int32_t x = packet.Read32();
					std::int32_t y = packet.Read32();
					std::string button = GetMouseButtonName(packet.Read8());
					if (packet.Read8()) {
						players[player->playerName].mouseButtons[button] = true;
						Lua_OnMouseButtonDown(player->playerName, x, y, button);
					}
					else {
						players[player->playerName].mouseButtons[button] = false;
						Lua_OnMouseButtonUp(player->playerName, x, y, button);
					}
				}
				std::int32_t mouseMotionX = packet.Read32();
				std::int32_t mouseMotionY = packet.Read32();
				if (packet.Read8()) {
					players[player->playerName].mouseX = mouseMotionX;
					players[player->playerName].mouseY = mouseMotionY;
					Lua_OnMouseMotion(player->playerName, mouseMotionX, mouseMotionY);
				}
			}
			enet_packet_destroy(event.packet);
			break;
		}
	}

	std::uint64_t now = SDL_GetTicks64();
	Lua_OnTick((now - lastTick) / 1000.0);
	lastTick = now;

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

	lua_newtable(L);
	lua_setglobal(L, "Game");

	lua_pushlightuserdata(L, this);
	lua_pushcclosure(L, Api_GetPlayers, 1);
	lua_setglobal(L, "GetPlayers");

	lua_pushlightuserdata(L, this);
	lua_pushcclosure(L, Api_IsKeyDown, 1);
	lua_setglobal(L, "IsKeyDown");

	lua_pushlightuserdata(L, this);
	lua_pushcclosure(L, Api_IsMouseButtonDown, 1);
	lua_setglobal(L, "IsMouseButtonDown");

	lua_pushlightuserdata(L, this);
	lua_pushcclosure(L, Api_GetMouseX, 1);
	lua_setglobal(L, "GetMouseX");

	lua_pushlightuserdata(L, this);
	lua_pushcclosure(L, Api_GetMouseY, 1);
	lua_setglobal(L, "GetMouseY");

	lua_pushlightuserdata(L, this);
	lua_pushcclosure(L, Api_DrawSprite, 1);
	lua_setglobal(L, "DrawSprite");

	lua_pushlightuserdata(L, this);
	lua_pushcclosure(L, Api_PlayerDrawSprite, 1);
	lua_setglobal(L, "PlayerDrawSprite");

	lua_pushlightuserdata(L, this);
	lua_pushcclosure(L, Api_IsPlayerOnline, 1);
	lua_setglobal(L, "IsPlayerOnline");

	if (luaL_dofile(L, path.c_str()) != LUA_OK) {
		std::cerr << "ERROR: Error while loading Lua script: " << lua_tostring(L, -1) << '\n';
	}
}

void Scene::Lua_OnTick(double dt) {
	lua_getglobal(L, "Game");
	lua_getfield(L, -1, "OnTick");

	if (lua_isnil(L, -1)) {
		lua_settop(L, 0);
		return;
	}

	lua_pushnumber(L, dt);

	if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
		std::cerr << "ERROR: Error while calling Game.OnTick: " << lua_tostring(L, -1) << '\n';
	}

	lua_settop(L, 0);
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

void Scene::Lua_OnKeyDown(const std::string& playerName, const std::string& key) {
	lua_getglobal(L, "Game");
	lua_getfield(L, -1, "OnKeyDown");

	if (lua_isnil(L, -1)) {
		lua_settop(L, 0);
		return;
	}

	lua_pushstring(L, playerName.c_str());
	lua_pushstring(L, key.c_str());

	if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
		std::cerr << "ERROR: Error while calling Game.OnKeyDown: " << lua_tostring(L, -1) << '\n';
	}

	lua_settop(L, 0);
}

void Scene::Lua_OnKeyUp(const std::string& playerName, const std::string& key) {
	lua_getglobal(L, "Game");
	lua_getfield(L, -1, "OnKeyUp");

	if (lua_isnil(L, -1)) {
		lua_settop(L, 0);
		return;
	}

	lua_pushstring(L, playerName.c_str());
	lua_pushstring(L, key.c_str());

	if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
		std::cerr << "ERROR: Error while calling Game.OnKeyUp: " << lua_tostring(L, -1) << '\n';
	}

	lua_settop(L, 0);
}

void Scene::Lua_OnMouseButtonDown(const std::string& playerName, std::int32_t x, std::int32_t y, const std::string& button) {
	lua_getglobal(L, "Game");
	lua_getfield(L, -1, "OnMouseButtonDown");

	if (lua_isnil(L, -1)) {
		lua_settop(L, 0);
		return;
	}

	lua_pushstring(L, playerName.c_str());
	lua_pushinteger(L, x);
	lua_pushinteger(L, y);
	lua_pushstring(L, button.c_str());

	if (lua_pcall(L, 4, 0, 0) != LUA_OK) {
		std::cerr << "ERROR: Error while calling Game.OnMouseButtonDown: " << lua_tostring(L, -1) << '\n';
	}

	lua_settop(L, 0);
}

void Scene::Lua_OnMouseButtonUp(const std::string& playerName, std::int32_t x, std::int32_t y, const std::string& button) {
	lua_getglobal(L, "Game");
	lua_getfield(L, -1, "OnMouseButtonUp");

	if (lua_isnil(L, -1)) {
		lua_settop(L, 0);
		return;
	}

	lua_pushstring(L, playerName.c_str());
	lua_pushinteger(L, x);
	lua_pushinteger(L, y);
	lua_pushstring(L, button.c_str());

	if (lua_pcall(L, 4, 0, 0) != LUA_OK) {
		std::cerr << "ERROR: Error while calling Game.OnMouseButtonUp: " << lua_tostring(L, -1) << '\n';
	}

	lua_settop(L, 0);
}

void Scene::Lua_OnMouseMotion(const std::string& playerName, std::int32_t x, std::int32_t y) {
	lua_getglobal(L, "Game");
	lua_getfield(L, -1, "OnMouseMotion");

	if (lua_isnil(L, -1)) {
		lua_settop(L, 0);
		return;
	}

	lua_pushstring(L, playerName.c_str());
	lua_pushinteger(L, x);
	lua_pushinteger(L, y);

	if (lua_pcall(L, 3, 0, 0) != LUA_OK) {
		std::cerr << "ERROR: Error while calling Game.OnMouseMotion: " << lua_tostring(L, -1) << '\n';
	}

	lua_settop(L, 0);
}

int Scene::Api_GetPlayers(lua_State* L) {
	Scene* scene = reinterpret_cast<Scene*>(lua_touserdata(L, lua_upvalueindex(1)));

	lua_createtable(L, static_cast<int>(scene->players.size()), 0);

	std::uint32_t i = 0;
	for (const auto& pair : scene->players) {
		lua_pushstring(L, pair.first.c_str());
		lua_rawseti(L, -2, i + 1);
		++i;
	}

	return 1;
}

int Scene::Api_IsKeyDown(lua_State* L) {
	Scene* scene = reinterpret_cast<Scene*>(lua_touserdata(L, lua_upvalueindex(1)));

	std::string playerName = luaL_checkstring(L, 1);
	std::string key = luaL_checkstring(L, 2);

	if (scene->players.find(playerName) == scene->players.end()) {
		luaL_error(L, "Player '%s' does not exist", playerName.c_str());
		return 0;
	}
	else {
		lua_pushboolean(L, scene->players[playerName].keys[key]);
		return 1;
	}
}

int Scene::Api_IsMouseButtonDown(lua_State* L) {
	Scene* scene = reinterpret_cast<Scene*>(lua_touserdata(L, lua_upvalueindex(1)));

	std::string playerName = luaL_checkstring(L, 1);
	std::string mouseButton = luaL_checkstring(L, 2);

	if (scene->players.find(playerName) == scene->players.end()) {
		luaL_error(L, "Player '%s' does not exist", playerName.c_str());
		return 0;
	}

	lua_pushboolean(L, scene->players[playerName].mouseButtons[mouseButton]);
	return 1;
}

int Scene::Api_GetMouseX(lua_State* L) {
	Scene* scene = reinterpret_cast<Scene*>(lua_touserdata(L, lua_upvalueindex(1)));

	std::string playerName = luaL_checkstring(L, 1);

	if (scene->players.find(playerName) == scene->players.end()) {
		luaL_error(L, "Player '%s' does not exist", playerName.c_str());
		return 0;
	}

	lua_pushinteger(L, scene->players[playerName].mouseX);
	return 1;
}

int Scene::Api_GetMouseY(lua_State* L) {
	Scene* scene = reinterpret_cast<Scene*>(lua_touserdata(L, lua_upvalueindex(1)));

	std::string playerName = luaL_checkstring(L, 1);

	if (scene->players.find(playerName) == scene->players.end()) {
		luaL_error(L, "Player '%s' does not exist", playerName.c_str());
		return 0;
	}

	lua_pushinteger(L, scene->players[playerName].mouseY);
	return 1;
}

int Scene::Api_DrawSprite(lua_State* L) {
	Scene* scene = reinterpret_cast<Scene*>(lua_touserdata(L, lua_upvalueindex(1)));

	std::string texture = luaL_checkstring(L, 1);
	std::int32_t x = static_cast<std::int32_t>(luaL_checknumber(L, 2));
	std::int32_t y = static_cast<std::int32_t>(luaL_checknumber(L, 3));
	std::uint32_t scale = static_cast<std::uint32_t>(luaL_checknumber(L, 4));

	if (scene->loadedTextures.find(texture) == scene->loadedTextures.end()) {
		luaL_error(L, "Texture '%s' is not loaded", texture.c_str());
		return 0;
	}

	for (auto& pair : scene->players) {
		pair.second.sprites.push_back({ x, y, scale, scene->loadedTextures[texture], 0 });
	}

	return 0;
}

int Scene::Api_PlayerDrawSprite(lua_State* L) {
	Scene* scene = reinterpret_cast<Scene*>(lua_touserdata(L, lua_upvalueindex(1)));

	std::string playerName = luaL_checkstring(L, 1);
	std::string texture = luaL_checkstring(L, 2);
	std::int32_t x = static_cast<std::int32_t>(luaL_checknumber(L, 3));
	std::int32_t y = static_cast<std::int32_t>(luaL_checknumber(L, 4));
	std::uint32_t scale = static_cast<std::uint32_t>(luaL_checknumber(L, 5));

	if (scene->players.find(playerName) == scene->players.end()) {
		luaL_error(L, "Player '%s' does not exist", playerName.c_str());
		return 0;
	}

	if (scene->loadedTextures.find(texture) == scene->loadedTextures.end()) {
		luaL_error(L, "Texture '%s' is not loaded", texture.c_str());
		return 0;
	}

	scene->players[playerName].sprites.push_back({ x, y, scale, scene->loadedTextures[texture], 0 });

	return 0;
}

int Scene::Api_IsPlayerOnline(lua_State* L) {
	Scene* scene = reinterpret_cast<Scene*>(lua_touserdata(L, lua_upvalueindex(1)));

	std::string playerName = luaL_checkstring(L, 1);

	lua_pushboolean(L, scene->players.find(playerName) != scene->players.end());
	return 1;
}
