// Copyright 2022 Justus Zorn

#ifndef Hazard_Scene_h
#define Hazard_Scene_h

#include <cstdint>
#include <string>

#include <enet.h>
#include <lua.hpp>

namespace Hazard {
	class Scene {
	public:
		Scene(std::string script, std::uint16_t port = 0);
		Scene(const Scene&) = delete;
		~Scene();

		Scene& operator=(const Scene&) = delete;

		void Update();

	private:
		ENetHost* host = nullptr;

		std::string path;
		lua_State* L = nullptr;

		void Reload();
	};
}

#endif
