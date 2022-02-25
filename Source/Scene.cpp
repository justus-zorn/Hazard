// Copyright 2022 Justus Zorn

#include <iostream>

#include "Scene.h"

using namespace Hazard;

Scene::Scene(std::string script) : path{ script } {
	L = luaL_newstate();
	if (!L) {
		std::cerr << "ERROR: Lua initialization failed\n";
		return;
	}

	luaL_openlibs(L);

	Reload();
}

Scene::~Scene() {
	lua_close(L);
}

void Scene::Reload() {
	if (luaL_dofile(L, path.c_str()) != LUA_OK) {
		std::cerr << "ERROR: Could not load Lua script: " << lua_tostring(L, -1) << '\n';
	}
}
