// Copyright 2022 Justus Zorn

#ifndef Hazard_Config_h
#define Hazard_Config_h

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

	private:
		std::string path;
		lua_State* L = nullptr;

		std::vector<std::string> textures;
	};
}

#endif
