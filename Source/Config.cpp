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
	textures.clear();
	windowTitle = "";
	windowWidth = 800;
	windowHeight = 800;
	maxPlayers = 32;
	port = 34344;

	lua_newtable(L);
	lua_setglobal(L, "Content");
	lua_newtable(L);
	lua_setglobal(L, "Window");
	lua_newtable(L);
	lua_setglobal(L, "Network");

	if (luaL_dofile(L, path.c_str()) != LUA_OK) {
		std::cerr << "ERROR: Error while loading Lua config: " << lua_tostring(L, -1) << '\n';
		return;
	}

	lua_getglobal(L, "Content");
	if (lua_isnil(L, -1)) {
		std::cerr << "ERROR: Content is nil\n";
	}
	else {
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
	}

	lua_settop(L, 0);

	lua_getglobal(L, "Window");
	if (lua_isnil(L, -1)) {
		std::cerr << "ERROR: Window is nil\n";
	}
	else {
		lua_getfield(L, -1, "title");

		if (!lua_isnil(L, -1)) {
			if (lua_isstring(L, -1)) {
				windowTitle = lua_tostring(L, -1);
			}
			else {
				std::cerr << "ERROR: Window.title is not a string\n";
			}
		}

		lua_pop(L, 1);
		lua_getfield(L, -1, "width");

		if (!lua_isnil(L, -1)) {
			if (lua_isinteger(L, -1)) {
				lua_Integer i = lua_tointeger(L, -1);
				if (i > 0) {
					windowWidth = static_cast<std::uint32_t>(i);
				}
				else {
					std::cerr << "ERROR: Window.width must be greater than 0\n";
				}
			}
			else {
				std::cerr << "ERROR: Window.width is not an integer\n";
			}
		}

		lua_pop(L, 1);
		lua_getfield(L, -1, "height");

		if (!lua_isnil(L, -1)) {
			if (lua_isinteger(L, -1)) {
				lua_Integer i = lua_tointeger(L, -1);
				if (i > 0) {
					windowHeight = static_cast<std::uint32_t>(i);
				}
				else {
					std::cerr << "ERROR: Window.height must be greater than 0\n";
				}
			}
			else {
				std::cerr << "ERROR: Window.height is not an integer\n";
			}
		}
	}

	lua_settop(L, 0);

	lua_getglobal(L, "Network");
	if (lua_isnil(L, -1)) {
		std::cerr << "ERROR: Network is nil\n";
	}
	else {
		lua_getfield(L, -1, "port");

		if (!lua_isnil(L, -1)) {
			if (lua_isinteger(L, -1)) {
				lua_Integer i = lua_tointeger(L, -1);
				if (i > 0 && i < UINT16_MAX) {
					port = static_cast<std::uint16_t>(i);
				}
				else {
					std::cerr << "ERROR: Port must be between 1 and 65535\n";
				}
			}
			else {
				std::cerr << "ERROR: Network.port is not an integer\n";
			}
		}

		lua_pop(L, 1);
		lua_getfield(L, -1, "max_players");
		
		if (!lua_isnil(L, -1)) {
			if (lua_isinteger(L, -1)) {
				lua_Integer i = lua_tointeger(L, -1);
				if (i > 0) {
					maxPlayers = static_cast<std::uint32_t>(i);
				}
				else {
					std::cerr << "ERROR: Network.max_players must be greater than 0\n";
				}
			}
			else {
				std::cerr << "ERROR: Network.max_players is not an integer\n";
			}
		}
	}
}

const std::vector<std::string>& Config::GetTextures() const {
	return textures;
}

const std::string& Config::WindowTitle() const {
	return windowTitle;
}

std::uint32_t Config::WindowWidth() const {
	return windowWidth;
}

std::uint32_t Config::WindowHeight() const {
	return windowHeight;
}

std::uint32_t Config::MaxPlayers() const {
	return maxPlayers;
}

std::uint16_t Config::Port() const {
	return port;
}
