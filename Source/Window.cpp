// Copyright 2022 Justus Zorn

#include <iostream>

#include "Window.h"

using namespace Hazard;

Window::Window(const std::string& title, int width, int height) {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cerr << "ERROR: Could not initialize SDL: " << SDL_GetError() << '\n';
		return;
	}

	window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, 0);
	if (!window) {
		std::cerr << "ERROR: Could not create window: " << SDL_GetError() << '\n';
		return;
	}
}

Window::~Window() {
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void Window::Update() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			shouldClose = true;
			break;
		}
	}
}

bool Window::ShouldClose() const {
	return shouldClose;
}
