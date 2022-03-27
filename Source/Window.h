// Copyright 2022 Justus Zorn

#ifndef Hazard_Window_h
#define Hazard_Window_h

#include <string>
#include <vector>

#include <glad/glad.h>
#include <SDL.h>

#include "Common.h"

namespace Hazard {
	class Window {
	public:
		Window(const std::string& title, std::uint32_t width, std::uint32_t height, std::uint32_t fontSize);
		Window(const Window&) = delete;
		~Window();

		Window& operator=(const Window&) = delete;

		bool Update();
		void Present();

		bool ShouldClose() const;

		void LoadTextures(const std::vector<std::string>& textures);
		void DrawSprite(const Sprite& sprite);

		const Input& GetInput() const;

		void SetTitle(const std::string& title);
		void SetSize(std::uint32_t width, std::uint32_t height);
		void ReloadFont(std::uint32_t fontSize);

	private:
		SDL_Window* window = nullptr;
		SDL_GLContext context = nullptr;
		/*TTF_Font* font = nullptr;*/

		bool shouldClose = false;

		struct Texture {
			GLuint id;
			int width, height;
		};

		std::vector<Texture> loadedTextures;
		GLuint program, positionUniform, sizeUniform, texcoordOffsetUniform, texcoordScaleUniform;
		GLuint vbo, vao;

		Input input;

		void FreeTextures();

		void CreateShader();
		void CreateVAO();
	};
}

#endif
