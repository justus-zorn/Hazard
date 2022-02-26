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
		const std::string& WindowTitle() const;
		std::uint32_t WindowWidth() const;
		std::uint32_t WindowHeight() const;
		std::uint16_t Port() const;

	private:
		std::string path;
		lua_State* L = nullptr;

		std::vector<std::string> textures;
		std::string windowTitle;
		std::uint32_t windowWidth, windowHeight;
		std::uint16_t port;
	};
}

#endif
