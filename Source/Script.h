// Copyright 2022 Justus Zorn

#ifndef Hazard_Script_h
#define Hazard_Script_h

#include <string>

#include <lua.hpp>

namespace Hazard {
	class Scene;

	class Script {
	public:
		Script(std::string path, Scene* scene);
		Script(const Script&) = delete;
		~Script();

		Script& operator=(const Script&) = delete;

		void Reload();

		void OnTick(double dt);
		
		bool OnLogin(const std::string& playerName);
		void OnJoin(const std::string& playerName);
		void OnDisconnect(const std::string& playerName);

		void OnKeyEvent(const std::string& playerName, const std::string& key, bool pressed);
		void OnButtonEvent(const std::string& playerName, const std::string& button, bool pressed);
		void OnAxisEvent(const std::string& playerName, const std::string& axis, std::int32_t state);

	private:
		std::string path;
		Scene* scene;

		lua_State* L;

		static int GetPlayers(lua_State* L);
		static int IsOnline(lua_State* L);
		static int Kick(lua_State* L);

		static int IsKeyDown(lua_State* L);
		static int IsButtonDown(lua_State* L);
		static int GetAxis(lua_State* L);

		static int DrawSprite(lua_State* L);
		static int DrawTextSprite(lua_State* L);

		static int GetTicks(lua_State* L);

		bool GetFunction(const std::string& function);
	};
}

#endif
