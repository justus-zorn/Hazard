// Copyright 2022 Justus Zorn

#include <iostream>

#include "Config.h"

using namespace Hazard;

Config::Config(std::string config) : path{ config } {
	L = luaL_newstate();
	if (!L) {
		std::cerr << "ERROR: Could not initialize Lua\n";
		return;
	}

	luaL_openlibs(L);

	Reload();
}

Config::~Config() {
	lua_close(L);
}

void Config::Reload() {
	lua_newtable(L);
	lua_setglobal(L, "Content");

	if (luaL_dofile(L, path.c_str()) != LUA_OK) {
		std::cerr << "ERROR: Error while loading Lua config: " << lua_tostring(L, -1) << '\n';
		return;
	}

	lua_getglobal(L, "Content");
	lua_getfield(L, -1, "textures");

	if (!lua_isnil(L, -1)) {
		if (lua_istable(L, -1)) {
			lua_pushnil(L);
			while (lua_next(L, -2)) {
				if (lua_isstring(L, -1)) {
					textures.push_back(lua_tostring(L, -1));
				}
				lua_pop(L, 1);
			}
		}
		else {
			std::cerr << "ERROR: Content.textures is not an array\n";
		}
	}

	lua_settop(L, 0);
}

const std::vector<std::string>& Config::GetTextures() const {
	return textures;
}
