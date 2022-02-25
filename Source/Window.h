// Copyright 2022 Justus Zorn

#ifndef Hazard_Window_h
#define Hazard_Window_h

#include <string>

#include <SDL.h>

namespace Hazard {
	class Window {
	public:
		Window(const std::string& title, int width, int height);
		Window(const Window&) = delete;
		~Window();

		Window& operator=(const Window&) = delete;

		void Update();
		bool ShouldClose() const;

	private:
		SDL_Window* window = nullptr;
		bool shouldClose = false;
	};
}

#endif
