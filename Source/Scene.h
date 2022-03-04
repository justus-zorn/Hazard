// Copyright 2022 Justus Zorn

#ifndef Hazard_Scene_h
#define Hazard_Scene_h

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <enet.h>

#include "Common.h"
#include "Config.h"
#include "Script.h"

namespace Hazard {
	class Scene {
	public:
		Scene(std::string script, Config& config, std::uint16_t port = 0);
		Scene(const Scene&) = delete;
		~Scene();

		Scene& operator=(const Scene&) = delete;

		void Update();

		void Reload();

		std::vector<std::string> GetPlayers();
		bool IsOnline(const std::string& playerName);
		void Kick(const std::string& playerName);

		bool KeyDown(const std::string& playerName, const std::string& key);
		bool ButtonDown(const std::string& playerName, const std::string& button);
		std::int32_t GetAxis(const std::string& playerName, const std::string& axis);

		bool IsTextureLoaded(const std::string& texture);
		void DrawSprite(const std::string& playerName, const std::string& texture, std::int32_t x, std::int32_t y, std::uint32_t scale, std::uint32_t animation);

	private:
		struct Player {
			std::string playerName;
			ENetPeer* peer;

			std::vector<Sprite> sprites;

			std::unordered_map<std::string, bool> keys;
			std::unordered_map<std::string, bool> buttons;

			std::int32_t mouseX = 0, mouseY = 0;
		};

		ENetHost* host = nullptr;

		Config& config;
		Script script;

		std::unordered_map<std::string, std::uint32_t> loadedTextures;

		std::unordered_map<std::string, Player> players;
		std::vector<std::string> kickedPlayers;

		std::uint64_t lastTicks;
	};
}

#endif
