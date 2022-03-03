// Copyright 2022 Justus Zorn

#include <iostream>

#include <SDL_image.h>

#include "Window.h"

using namespace Hazard;

Window::Window(const std::string& title, std::uint32_t width, std::uint32_t height) {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
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

	int img_flags = IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_WEBP;
	if (IMG_Init(img_flags) != img_flags) {
		std::cerr << "ERROR: Could not load SDL_image: " << IMG_GetError() << '\n';
	}

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
}

Window::~Window() {
	FreeTextures();

	IMG_Quit();

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
		SDL_Surface* surface = IMG_Load(path.c_str());
		if (surface) {
			SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
			SDL_FreeSurface(surface);
			if (texture) {
				loadedTextures.push_back(texture);
				continue;
			}
		}
		std::cerr << "ERROR: Could not load texture '" << path << "': " << SDL_GetError() << '\n';
		loadedTextures.push_back(nullptr);
	}
}

void Window::DrawSprite(const Sprite& sprite) {
	if (sprite.texture >= loadedTextures.size() || !loadedTextures[sprite.texture]) {
		return;
	}

	SDL_Texture* texture = loadedTextures[sprite.texture];
	int textureWidth, textureHeight;

	SDL_QueryTexture(texture, nullptr, nullptr, &textureWidth, &textureHeight);

	int windowWidth, windowHeight;
	SDL_GetWindowSize(window, &windowWidth, &windowHeight);

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

const Input& Window::GetInput() const {
	return input;
}

void Window::SetTitle(const std::string& title) {
	SDL_SetWindowTitle(window, title.c_str());
}

void Window::SetSize(std::uint32_t width, std::uint32_t height) {
	SDL_SetWindowSize(window, width, height);
	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

void Window::FreeTextures() {
	for (SDL_Texture* texture : loadedTextures) {
		SDL_DestroyTexture(texture);
	}
	loadedTextures.clear();
}
