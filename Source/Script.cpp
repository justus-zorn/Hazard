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
	lua_setglobal(L, "GetPlayers");

	lua_pushlightuserdata(L, scene);
	lua_pushcclosure(L, IsOnline, 1);
	lua_setglobal(L, "IsOnline");

	lua_pushlightuserdata(L, scene);
	lua_pushcclosure(L, Kick, 1);
	lua_setglobal(L, "Kick");

	lua_pushlightuserdata(L, scene);
	lua_pushcclosure(L, KeyDown, 1);
	lua_setglobal(L, "KeyDown");

	lua_pushlightuserdata(L, scene);
	lua_pushcclosure(L, ButtonDown, 1);
	lua_setglobal(L, "ButtonDown");

	lua_pushlightuserdata(L, scene);
	lua_pushcclosure(L, GetAxis, 1);
	lua_setglobal(L, "GetAxis");

	lua_pushlightuserdata(L, scene);
	lua_pushcclosure(L, DrawSprite, 1);
	lua_setglobal(L, "DrawSprite");

	lua_pushcclosure(L, GetTicks, 0);
	lua_setglobal(L, "GetTicks");

	if (luaL_dofile(L, path.c_str()) != LUA_OK) {
		std::cerr << "ERROR: Error while loading Lua script: " << lua_tostring(L, -1) << '\n';
	}
}

void Script::OnTick(double dt) {
	lua_getglobal(L, "Game");
	if (!lua_isnil(L, -1)) {
		lua_getfield(L, -1, "OnTick");

		if (!lua_isnil(L, -1)) {
			lua_pushnumber(L, dt);
			if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
				std::cerr << "ERROR: Error while calling Game.OnTick: " << lua_tostring(L, -1) << '\n';
			}
		}
	}
	else {
		std::cerr << "ERROR: Error while calling Game.OnTick: 'Game' is nil\n";
	}
	
	lua_settop(L, 0);
}

bool Script::OnPreLogin(const std::string& playerName) {
	bool result = false;
	if (GetFunction("OnPreLogin")) {
		lua_pushstring(L, playerName.c_str());
		if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
			std::cerr << "ERROR: Error while calling Game.OnPreLogin: " << lua_tostring(L, -1) << '\n';
		}
		else {
			if (!lua_isboolean(L, -1)) {
				std::cerr << "ERROR: Error while calling Game.OnPreLogin: Function must return a boolean\n";
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

void Script::OnPostLogin(const std::string& playerName) {
	if (GetFunction("OnPostLogin")) {
		lua_pushstring(L, playerName.c_str());
		if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
			std::cerr << "ERROR: Error while calling Game.OnPostLogin: " << lua_tostring(L, -1) << '\n';
		}
	}

	lua_settop(L, 0);
}

void Script::OnDisconnect(const std::string& playerName) {
	if (GetFunction("OnDisconnect")) {
		lua_pushstring(L, playerName.c_str());
		if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
			std::cerr << "ERROR: Error while calling Game.OnDisconnect: " << lua_tostring(L, -1) << '\n';
		}
	}

	lua_settop(L, 0);
}

void Script::OnKeyEvent(const std::string& playerName, const std::string& key, bool pressed) {
	if (GetFunction("OnKeyEvent")) {
		lua_pushstring(L, playerName.c_str());
		lua_pushstring(L, key.c_str());
		lua_pushboolean(L, pressed);
		if (lua_pcall(L, 3, 0, 0) != LUA_OK) {
			std::cerr << "ERROR: Error while calling Game.OnKeyEvent: " << lua_tostring(L, -1) << '\n';
		}
	}

	lua_settop(L, 0);
}

void Script::OnButtonEvent(const std::string& playerName, const std::string& button, bool pressed) {
	if (GetFunction("OnButtonEvent")) {
		lua_pushstring(L, playerName.c_str());
		lua_pushstring(L, button.c_str());
		lua_pushboolean(L, pressed);
		if (lua_pcall(L, 3, 0, 0) != LUA_OK) {
			std::cerr << "ERROR: Error while calling Game.OnButtonEvent: " << lua_tostring(L, -1) << '\n';
		}
	}

	lua_settop(L, 0);
}

void Script::OnAxisEvent(const std::string& playerName, const std::string& axis, std::int32_t state) {
	if (GetFunction("OnAxisEvent")) {
		lua_pushstring(L, playerName.c_str());
		lua_pushstring(L, axis.c_str());
		lua_pushinteger(L, state);
		if (lua_pcall(L, 3, 0, 0) != LUA_OK) {
			std::cerr << "ERROR: Error while calling Game.OnAxisEvent: " << lua_tostring(L, -1) << '\n';
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

int Script::KeyDown(lua_State* L) {
	Scene* scene = reinterpret_cast<Scene*>(lua_touserdata(L, lua_upvalueindex(1)));
	std::string playerName = luaL_checkstring(L, 1);
	if (!scene->IsOnline(playerName)) {
		return luaL_error(L, "Player %s is not online", playerName.c_str());
	}
	std::string key = luaL_checkstring(L, 2);
	lua_pushboolean(L, scene->KeyDown(playerName, key));
	return 1;
}

int Script::ButtonDown(lua_State* L) {
	Scene* scene = reinterpret_cast<Scene*>(lua_touserdata(L, lua_upvalueindex(1)));
	std::string playerName = luaL_checkstring(L, 1);
	if (!scene->IsOnline(playerName)) {
		return luaL_error(L, "Player %s is not online", playerName.c_str());
	}
	std::string button = luaL_checkstring(L, 2);
	lua_pushboolean(L, scene->ButtonDown(playerName, button));
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
	std::uint32_t scale = static_cast<std::uint32_t>(luaL_checknumber(L, 5));
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
