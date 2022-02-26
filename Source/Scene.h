// Copyright 2022 Justus Zorn

#ifndef Hazard_Scene_h
#define Hazard_Scene_h

#include <cstdint>
#include <string>
#include <unordered_map>

#include <enet.h>
#include <lua.hpp>

#include "Common.h"
#include "Config.h"

namespace Hazard {
	class Scene {
	public:
		Scene(std::string script, Config& config, std::uint16_t port = 0);
		Scene(const Scene&) = delete;
		~Scene();

		Scene& operator=(const Scene&) = delete;

		void Update();

	private:
		struct Player {
			std::string playerName;
			ENetPeer* peer;
			std::vector<Sprite> sprites;

			std::unordered_map<std::string, bool> keys;
			std::unordered_map<std::string, bool> mouseButtons;

			std::int32_t mouseX = 0, mouseY = 0;
		};

		Config& config;
		std::unordered_map<std::string, std::uint32_t> loadedTextures;

		ENetHost* host = nullptr;

		std::string path;
		lua_State* L = nullptr;

		std::unordered_map<std::string, Player> players;

		std::uint64_t lastTick;

		void Reload();

		void Lua_OnTick(double dt);

		bool Lua_OnPreLogin(const std::string& playerName);
		void Lua_OnPostLogin(const std::string& playerName);
		void Lua_OnDisconnect(const std::string& playerName);

		void Lua_OnKeyDown(const std::string& playerName, const std::string& key);
		void Lua_OnKeyUp(const std::string& playerName, const std::string& key);
		void Lua_OnMouseButtonDown(const std::string& playerName, std::int32_t x, std::int32_t y, const std::string& key);
		void Lua_OnMouseButtonUp(const std::string& playerName, std::int32_t x, std::int32_t y, const std::string& key);
		void Lua_OnMouseMotion(const std::string& playerName, std::int32_t x, std::int32_t y);

		static int Api_GetPlayers(lua_State* L);
		static int Api_IsKeyDown(lua_State* L);
		static int Api_IsMouseButtonDown(lua_State* L);

		static int Api_GetMouseX(lua_State* L);
		static int Api_GetMouseY(lua_State* L);

		static int Api_DrawSprite(lua_State* L);
		static int Api_PlayerDrawSprite(lua_State* L);
		static int Api_IsPlayerOnline(lua_State* L);
	};
}

#endif
