// Copyright 2022 Justus Zorn

#include <iostream>

#include <stb_image.h>

#include "Window.h"

using namespace Hazard;

Window::Window(const std::string& title, std::uint32_t width, std::uint32_t height, std::uint32_t fontSize) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		std::cerr << "ERROR: Could not initialize SDL: " << SDL_GetError() << '\n';
		return;
	}

	window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, 0);
	if (!window) {
		std::cerr << "ERROR: Could not create window: " << SDL_GetError() << '\n';
		return;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer) {
		std::cerr << "ERROR: Could not create renderer: " << SDL_GetError() << '\n';
		return;
	}

	if (TTF_Init() < 0) {
		std::cerr << "ERROR: Could not initialize SDL_ttf: " << TTF_GetError() << '\n';
	}
	else {
		font = TTF_OpenFont("font.ttf", fontSize);
		if (!font) {
			std::cerr << "ERROR: Could not load 'font.ttf': " << TTF_GetError() << '\n';
		}
	}

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	SDL_StartTextInput();
}

Window::~Window() {
	SDL_StopTextInput();

	FreeTextures();

	TTF_CloseFont(font);
	TTF_Quit();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
}

bool Window::Update() {
	input.Clear();

	bool shouldReload = false;

	int windowWidth, windowHeight;
	SDL_GetWindowSize(window, &windowWidth, &windowHeight);

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			shouldClose = true;
			break;
		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_F5) {
				shouldReload = true;
			}
			input.keyboardInputs.push_back({ event.key.keysym.sym, true });
			break;
		case SDL_KEYUP:
			input.keyboardInputs.push_back({ event.key.keysym.sym, false });
			break;
		case SDL_MOUSEBUTTONDOWN:
			input.buttonInputs.push_back({ event.button.button, true });
			break;
		case SDL_MOUSEBUTTONUP:
			input.buttonInputs.push_back({ event.button.button, false });
			break;
		case SDL_MOUSEMOTION:
			input.mouseMotionX = event.motion.x - windowWidth / 2;
			input.mouseMotionY = windowHeight / 2 - event.motion.y;
			input.mouseMotion = true;
			break;
		case SDL_TEXTINPUT:
			input.textInput += event.text.text;
			break;
		}
	}

	return shouldReload;
}

void Window::Present() {
	SDL_RenderPresent(renderer);
	SDL_RenderClear(renderer);
}

bool Window::ShouldClose() const {
	return shouldClose;
}

void Window::LoadTextures(const std::vector<std::string>& textures) {
	FreeTextures();

	for (const std::string& texture : textures) {
		std::string path = "Textures/" + texture;
		int width, height;
		unsigned char* data = stbi_load(path.c_str(), &width, &height, nullptr, STBI_rgb_alpha);
		if (!data) {
			std::cerr << "ERROR: Could not load texture '" << path << "'\n";
			loadedTextures.push_back(nullptr);
			continue;
		}

		SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
		if (texture) {
			if (SDL_UpdateTexture(texture, nullptr, data, width * 4) >= 0) {
				loadedTextures.push_back(texture);
				stbi_image_free(data);
				continue;
			}
			SDL_DestroyTexture(texture);
		}
		std::cerr << "ERROR: Could not load texture '" << path << "': " << SDL_GetError() << '\n';
		loadedTextures.push_back(nullptr);
		stbi_image_free(data);
	}
}

void Window::DrawSprite(const Sprite& sprite) {
	int windowWidth, windowHeight;
	SDL_GetWindowSize(window, &windowWidth, &windowHeight);

	if (sprite.isText) {
		if (!font || sprite.text.length() == 0) {
			return;
		}
		SDL_Surface* surface = TTF_RenderUTF8_Blended_Wrapped(font, sprite.text.c_str(), { sprite.r, sprite.g, sprite.b }, sprite.scale);
		if (!surface) {
			std::cerr << "ERROR: Text rendering failed: " << TTF_GetError() << '\n';
			return;
		}
		SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_FreeSurface(surface);
		if (!texture) {
			std::cerr << "ERROR: Text rendering failed: " << SDL_GetError() << '\n';
			return;
		}

		int textureWidth, textureHeight;
		SDL_QueryTexture(texture, nullptr, nullptr, &textureWidth, &textureHeight);

		SDL_Rect dst;
		dst.x = (windowWidth / 2) + (sprite.x - textureWidth / 2);
		dst.y = (windowHeight / 2) - (sprite.y + textureHeight / 2);
		dst.w = textureWidth;
		dst.h = textureHeight;

		SDL_RenderCopy(renderer, texture, nullptr, &dst);

		SDL_DestroyTexture(texture);
	}
	else {
		if (sprite.texture >= loadedTextures.size() || !loadedTextures[sprite.texture]) {
			return;
		}

		SDL_Texture* texture = loadedTextures[sprite.texture];
		int textureWidth, textureHeight;

		SDL_QueryTexture(texture, nullptr, nullptr, &textureWidth, &textureHeight);

		std::uint32_t animation = sprite.animation % (textureWidth / textureHeight);
		SDL_Rect src;
		src.x = textureHeight * animation;
		src.y = 0;
		src.w = textureHeight;
		src.h = textureHeight;

		SDL_Rect dst;
		dst.x = (windowWidth / 2) + (sprite.x - sprite.scale);
		dst.y = (windowHeight / 2) - (sprite.y + sprite.scale);
		dst.w = sprite.scale * 2;
		dst.h = sprite.scale * 2;

		SDL_RenderCopy(renderer, texture, &src, &dst);
	}
}

const Input& Window::GetInput() const {
	return input;
}

void Window::SetTitle(const std::string& title) {
	SDL_SetWindowTitle(window, title.c_str());
}

void Window::SetSize(std::uint32_t width, std::uint32_t height) {
	SDL_SetWindowSize(window, width, height);
}

void Window::ReloadFont(std::uint32_t fontSize) {
	TTF_CloseFont(font);
	font = TTF_OpenFont("font.ttf", fontSize);
	if (!font) {
		std::cerr << "ERROR: Could not load 'font.ttf': " << TTF_GetError() << '\n';
	}
}

void Window::FreeTextures() {
	for (SDL_Texture* texture : loadedTextures) {
		if (texture) {
			SDL_DestroyTexture(texture);
		}
	}
	loadedTextures.clear();
}
