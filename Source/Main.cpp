// Copyright 2022 Justus Zorn

#include <atomic>
#include <iostream>
#include <thread>

#include <enet.h>

#include "Client.h"
#include "Config.h"
#include "Scene.h"
#include "Window.h"

using namespace Hazard;

static std::atomic<bool> running = true;
static std::atomic<bool> shouldReload = false;

void RunClient(const std::string& player, const std::string& address) {
	Config config("config.lua");
	Client client(player, address, config.Port());
	Window window(config.WindowTitle(), config.WindowWidth(), config.WindowHeight(), config.FontSize());

	window.LoadTextures(config.GetTextures());
	while (!window.ShouldClose()) {
		if (window.Update()) {
			shouldReload = true;
			config.Reload();
			window.SetTitle(config.WindowTitle());
			window.SetSize(config.WindowWidth(), config.WindowHeight());
			window.ReloadFont(config.FontSize());
			window.LoadTextures(config.GetTextures());
		}
		if (!client.Update(window.GetInput())) {
			break;
		}
		for (const Sprite& sprite : client.GetSprites()) {
			window.DrawSprite(sprite);
		}
		window.Present();
	}
}

void RunServer() {
	Config config("config.lua");
	Scene scene("main.lua", config);
	while (running) {
		if (shouldReload) {
			scene.Reload();
			shouldReload = false;
		}

		std::uint64_t start = SDL_GetTicks64();
		scene.Update();
		std::uint64_t ticks = SDL_GetTicks64() - start;
		if (ticks < 16) {
			SDL_Delay(static_cast<std::uint32_t>(16 - ticks));
		}
	}
}

int main(int argc, char* argv[]) {
	if (enet_initialize() < 0) {
		std::cerr << "ERROR: Could not initialize ENet\n";
		return 1;
	}

	if (argc == 1) {
		std::thread server(RunServer);

		RunClient("local", "localhost");

		running = false;
		server.join();
	}
	else {
		if (std::string(argv[1]) == "--connect") {
			if (argc < 4) {
				std::cerr << "ERROR: Missing command line arguments\n";
			}
			else {
				RunClient(argv[3], argv[2]);
			}
		}
		else if (std::string(argv[1]) == "--server") {
			RunServer();
		}
		else {
			std::cerr << "ERROR: Unknown command line option '" << argv[1] << '\n';
		}
	}

	enet_deinitialize();

	return 0;
}
