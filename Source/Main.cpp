// Copyright 2022 Justus Zorn

#include <iostream>

#include <enet.h>

#include "Client.h"
#include "Config.h"
#include "Scene.h"
#include "Window.h"

using namespace Hazard;

int main(int argc, char* argv[]) {
	if (enet_initialize() < 0) {
		std::cerr << "ERROR: Could not initialize ENet\n";
		return 1;
	}

	if (argc > 1) {
		Config config("config.lua");
		Client client(argv[2], argv[1], 34344);
		Window window("My window", 1280, 720);

		window.LoadTextures(config.GetTextures());
		while (!window.ShouldClose()) {
			window.Update();
			if (!client.Update(window.GetInput())) {
				break;
			}
			for (const Sprite& sprite : client.GetSprites()) {
				window.DrawSprite(sprite);
			}
			window.Present();
		}
	}
	else {
		Config config("config.lua");
		Scene scene("main.lua", config);
		while (true) {
			std::uint64_t start = SDL_GetTicks64();
			scene.Update();
			std::uint64_t ticks = SDL_GetTicks64() - start;
			if (ticks < 16) {
				SDL_Delay(static_cast<std::uint32_t>(16 - ticks));
			}
		}
	}

	enet_deinitialize();

	return 0;
}
