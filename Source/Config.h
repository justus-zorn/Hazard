// Copyright 2022 Justus Zorn

#ifndef Hazard_Config_h
#define Hazard_Config_h

#include <cstdint>
#include <string>
#include <vector>

#include <lua.hpp>

namespace Hazard {
	class Config {
	public:
		Config(std::string config);
		Config(const Config&) = delete;
		~Config();

		Config& operator=(const Config&) = delete;

		void Reload();

		const std::vector<std::string>& GetTextures() const;
		const std::vector<std::string>& GetSounds() const;

		const std::string& WindowTitle() const;
		std::uint32_t WindowWidth() const;
		std::uint32_t WindowHeight() const;
		std::uint32_t FontSize() const;
		std::uint16_t Port() const;
		std::uint32_t MaxPlayers() const;

	private:
		std::string path;
		lua_State* L = nullptr;

		std::vector<std::string> textures;
		std::vector<std::string> sounds;

		std::string windowTitle;
		std::uint32_t windowWidth, windowHeight;
		std::uint32_t fontSize;
		std::uint16_t port;
		std::uint32_t maxPlayers;
	};
}

#endif
