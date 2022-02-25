// Copyright 2022 Justus Zorn

#ifndef Hazard_Window_h
#define Hazard_Window_h

#include <string>
#include <vector>

#include <SDL.h>

#include "Common.h"

namespace Hazard {
	class Window {
	public:
		Window(const std::string& title, int width, int height);
		Window(const Window&) = delete;
		~Window();

		Window& operator=(const Window&) = delete;

		void Update();
		void Present();

		bool ShouldClose() const;

		void LoadTextures(const std::vector<std::string>& textures);

		void DrawSprite(const Sprite& sprite);

	private:
		SDL_Window* window = nullptr;
		SDL_Renderer* renderer = nullptr;
		bool shouldClose = false;

		std::vector<SDL_Texture*> loadedTextures;

		void FreeTextures();
	};
}

#endif
