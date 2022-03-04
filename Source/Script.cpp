// Copyright 2022 Justus Zorn

#include <iostream>
#include <vector>

#include <SDL.h>

#include "Scene.h"
#include "Script.h"

using namespace Hazard;

Script::Script(std::string path, Scene* scene) : path{ path }, scene{ scene } {
	L = luaL_newstate();
	if (!L) {
		std::cerr << "ERROR: Lua initialization failed\n";
		return;
	}

	luaL_openlibs(L);

	Reload();
}

Script::~Script() {
	lua_close(L);
}

void Script::Reload() {
	lua_newtable(L);
	lua_setglobal(L, "Game");

	lua_pushlightuserdata(L, scene);
	lua_pushcclosure(L, GetPlayers, 1);
	lua_setglobal(L, "get_players");

	lua_pushlightuserdata(L, scene);
	lua_pushcclosure(L, IsOnline, 1);
	lua_setglobal(L, "is_online");

	lua_pushlightuserdata(L, scene);
	lua_pushcclosure(L, Kick, 1);
	lua_setglobal(L, "kick");

	lua_pushlightuserdata(L, scene);
	lua_pushcclosure(L, IsKeyDown, 1);
	lua_setglobal(L, "is_key_down");

	lua_pushlightuserdata(L, scene);
	lua_pushcclosure(L, IsButtonDown, 1);
	lua_setglobal(L, "is_button_down");

	lua_pushlightuserdata(L, scene);
	lua_pushcclosure(L, GetAxis, 1);
	lua_setglobal(L, "get_axis");

	lua_pushlightuserdata(L, scene);
	lua_pushcclosure(L, DrawSprite, 1);
	lua_setglobal(L, "draw_sprite");

	lua_pushcclosure(L, GetTicks, 0);
	lua_setglobal(L, "get_ticks");

	if (luaL_dofile(L, path.c_str()) != LUA_OK) {
		std::cerr << "ERROR: Error while loading Lua script: " << lua_tostring(L, -1) << '\n';
	}
}

void Script::OnTick(double dt) {
	if (GetFunction("on_tick")) {
		lua_pushnumber(L, dt);
		if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
			std::cerr << "ERROR: Error while calling Game.on_tick: " << lua_tostring(L, -1) << '\n';
		}
	}
	
	lua_settop(L, 0);
}

bool Script::OnLogin(const std::string& playerName) {
	bool result = false;
	if (GetFunction("on_login")) {
		lua_pushstring(L, playerName.c_str());
		if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
			std::cerr << "ERROR: Error while calling Game.on_login: " << lua_tostring(L, -1) << '\n';
		}
		else {
			if (!lua_isboolean(L, -1)) {
				std::cerr << "ERROR: Error while calling Game.on_login: Function must return a boolean\n";
			}
			else {
				result = lua_toboolean(L, -1);
			}
		}
	}
	else {
		result = true;
	}

	lua_settop(L, 0);
	return result;
}

void Script::OnJoin(const std::string& playerName) {
	if (GetFunction("on_join")) {
		lua_pushstring(L, playerName.c_str());
		if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
			std::cerr << "ERROR: Error while calling Game.on_join: " << lua_tostring(L, -1) << '\n';
		}
	}

	lua_settop(L, 0);
}

void Script::OnDisconnect(const std::string& playerName) {
	if (GetFunction("on_disconnect")) {
		lua_pushstring(L, playerName.c_str());
		if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
			std::cerr << "ERROR: Error while calling Game.on_disconnect: " << lua_tostring(L, -1) << '\n';
		}
	}

	lua_settop(L, 0);
}

void Script::OnKeyEvent(const std::string& playerName, const std::string& key, bool pressed) {
	if (GetFunction("on_key_event")) {
		lua_pushstring(L, playerName.c_str());
		lua_pushstring(L, key.c_str());
		lua_pushboolean(L, pressed);
		if (lua_pcall(L, 3, 0, 0) != LUA_OK) {
			std::cerr << "ERROR: Error while calling Game.on_key_event: " << lua_tostring(L, -1) << '\n';
		}
	}

	lua_settop(L, 0);
}

void Script::OnButtonEvent(const std::string& playerName, const std::string& button, bool pressed) {
	if (GetFunction("on_button_event")) {
		lua_pushstring(L, playerName.c_str());
		lua_pushstring(L, button.c_str());
		lua_pushboolean(L, pressed);
		if (lua_pcall(L, 3, 0, 0) != LUA_OK) {
			std::cerr << "ERROR: Error while calling Game.on_button_event: " << lua_tostring(L, -1) << '\n';
		}
	}

	lua_settop(L, 0);
}

void Script::OnAxisEvent(const std::string& playerName, const std::string& axis, std::int32_t state) {
	if (GetFunction("on_axis_event")) {
		lua_pushstring(L, playerName.c_str());
		lua_pushstring(L, axis.c_str());
		lua_pushinteger(L, state);
		if (lua_pcall(L, 3, 0, 0) != LUA_OK) {
			std::cerr << "ERROR: Error while calling Game.on_axis_event: " << lua_tostring(L, -1) << '\n';
		}
	}

	lua_settop(L, 0);
}

int Script::GetPlayers(lua_State* L) {
	Scene* scene = reinterpret_cast<Scene*>(lua_touserdata(L, lua_upvalueindex(1)));
	std::vector<std::string> players = scene->GetPlayers();

	lua_createtable(L, static_cast<int>(players.size()), 0);

	std::uint32_t i = 1;
	for (const std::string& player : players) {
		lua_pushstring(L, player.c_str());
		lua_rawseti(L, -2, i);
		++i;
	}

	return 1;
}

int Script::IsOnline(lua_State* L) {
	Scene* scene = reinterpret_cast<Scene*>(lua_touserdata(L, lua_upvalueindex(1)));
	std::string playerName = luaL_checkstring(L, 1);
	lua_pushboolean(L, scene->IsOnline(playerName));
	return 1;
}

int Script::Kick(lua_State* L) {
	Scene* scene = reinterpret_cast<Scene*>(lua_touserdata(L, lua_upvalueindex(1)));
	std::string playerName = luaL_checkstring(L, 1);
	if (!scene->IsOnline(playerName)) {
		return luaL_error(L, "Player %s is not online", playerName.c_str());
	}
	scene->Kick(playerName);
	return 0;
}

int Script::IsKeyDown(lua_State* L) {
	Scene* scene = reinterpret_cast<Scene*>(lua_touserdata(L, lua_upvalueindex(1)));
	std::string playerName = luaL_checkstring(L, 1);
	if (!scene->IsOnline(playerName)) {
		return luaL_error(L, "Player %s is not online", playerName.c_str());
	}
	std::string key = luaL_checkstring(L, 2);
	lua_pushboolean(L, scene->IsKeyDown(playerName, key));
	return 1;
}

int Script::IsButtonDown(lua_State* L) {
	Scene* scene = reinterpret_cast<Scene*>(lua_touserdata(L, lua_upvalueindex(1)));
	std::string playerName = luaL_checkstring(L, 1);
	if (!scene->IsOnline(playerName)) {
		return luaL_error(L, "Player %s is not online", playerName.c_str());
	}
	std::string button = luaL_checkstring(L, 2);
	lua_pushboolean(L, scene->IsButtonDown(playerName, button));
	return 1;
}

int Script::GetAxis(lua_State* L) {
	Scene* scene = reinterpret_cast<Scene*>(lua_touserdata(L, lua_upvalueindex(1)));
	std::string playerName = luaL_checkstring(L, 1);
	if (!scene->IsOnline(playerName)) {
		return luaL_error(L, "Player %s is not online", playerName.c_str());
	}
	std::string axis = luaL_checkstring(L, 2);
	lua_pushinteger(L, scene->GetAxis(playerName, axis));
	return 1;
}

int Script::DrawSprite(lua_State* L) {
	Scene* scene = reinterpret_cast<Scene*>(lua_touserdata(L, lua_upvalueindex(1)));
	std::string playerName = luaL_checkstring(L, 1);
	if (!scene->IsOnline(playerName)) {
		return luaL_error(L, "Player %s is not online", playerName.c_str());
	}
	std::string texture = luaL_checkstring(L, 2);
	if (!scene->IsTextureLoaded(texture)) {
		return luaL_error(L, "Texture %s is not loaded", texture.c_str());
	}
	std::int32_t x = static_cast<std::int32_t>(luaL_checknumber(L, 3));
	std::int32_t y = static_cast<std::int32_t>(luaL_checknumber(L, 4));
	std::uint32_t scale = static_cast<std::uint32_t>(luaL_checknumber(L, 5)) / 2;
	std::uint32_t animation = 0;
	if (lua_gettop(L) > 5) {
		std::uint32_t frameTime = static_cast<std::uint32_t>(luaL_checknumber(L, 6));
		std::uint32_t start = 0;
		if (lua_gettop(L) > 6) {
			start = static_cast<std::uint32_t>(luaL_checknumber(L, 7));
		}
		animation = static_cast<std::uint32_t>((SDL_GetTicks64() - start) / frameTime);
	}

	scene->DrawSprite(playerName, texture, x, y, scale, animation);

	return 0;
}

int Script::GetTicks(lua_State* L) {
	lua_pushinteger(L, static_cast<lua_Integer>(SDL_GetTicks64()));
	return 1;
}

bool Script::GetFunction(const std::string& function) {
	lua_getglobal(L, "Game");
	if (lua_isnil(L, -1)) {
		std::cerr << "ERROR: 'Game' is nil\n";
		return false;
	}
	lua_getfield(L, -1, function.c_str());
	if (lua_isnil(L, -1)) {
		return false;
	}
	return true;
}
