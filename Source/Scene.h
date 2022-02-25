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
			std::string playerName;
			ENetPeer* peer;
		};

		ENetHost* host = nullptr;

		std::string path;
		lua_State* L = nullptr;

		std::unordered_map<std::string, Player> players;

		void Reload();

		bool Lua_OnPreLogin(const std::string& playerName);
		void Lua_OnPostLogin(const std::string& playerName);
		void Lua_OnDisconnect(const std::string& playerName);
	};
}

#endif
