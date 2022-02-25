// Copyright 2022 Justus Zorn

#ifndef Hazard_Scene_h
#define Hazard_Scene_h

#include <cstdint>
#include <string>
#include <unordered_map>

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
		struct Player {
			std::string player_name;
			ENetPeer* peer;
		};

		ENetHost* host = nullptr;

		std::string path;
		lua_State* L = nullptr;

		std::unordered_map<std::string, Player> players;

		void Reload();
	};
}

#endif
