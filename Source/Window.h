// Copyright 2022 Justus Zorn

#ifndef Hazard_Window_h
#define Hazard_Window_h

#include <string>
#include <vector>

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

#include "Common.h"

namespace Hazard {
	class Window {
	public:
		Window(const std::string& title, std::uint32_t width, std::uint32_t height, std::uint32_t fontSize, std::uint16_t channels);
		Window(const Window&) = delete;
		~Window();

		Window& operator=(const Window&) = delete;

		bool Update();
		void Present();

		bool ShouldClose() const;

		void LoadTextures(const std::vector<std::string>& textures);
		void LoadSounds(const std::vector<std::string>& sounds);

		void DrawSprite(const Sprite& sprite);

		void Audio(const AudioCommand& audioCommand);

		const Input& GetInput() const;

		void SetTitle(const std::string& title);
		void SetSize(std::uint32_t width, std::uint32_t height);
		void ReloadFont(std::uint32_t fontSize);
		void SetChannels(std::uint16_t newChannels);

	private:
		SDL_Window* window = nullptr;
		SDL_Renderer* renderer = nullptr;
		TTF_Font* font = nullptr;

		std::uint16_t channels;

		bool shouldClose = false;

		std::vector<SDL_Texture*> loadedTextures;
		std::vector<Mix_Chunk*> loadedSounds;

		std::string composition;
		Input input;

		void FreeTextures();
		void FreeSounds();
	};
}

#endif
