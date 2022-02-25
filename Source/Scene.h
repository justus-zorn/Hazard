// Copyright 2022 Justus Zorn

#ifndef Hazard_Scene_h
#define Hazard_Scene_h

#include <cstdint>
#include <string>

#include <lua.hpp>

namespace Hazard {
	class Scene {
	public:
		Scene(std::string script);
		Scene(const Scene&) = delete;
		~Scene();

		Scene& operator=(const Scene&) = delete;

	private:
		std::string path;
		lua_State* L;

		void Reload();
	};
}

#endif
