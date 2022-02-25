// Copyright 2022 Justus Zorn

#include <iostream>

#include <enet.h>

#include "Client.h"
#include "Scene.h"
#include "Window.h"

using namespace Hazard;

int main(int argc, char* argv[]) {
	if (enet_initialize() < 0) {
		std::cerr << "ERROR: Could not initialize ENet\n";
		return 1;
	}

	if (argc > 1) {
		Client client(argv[2], argv[1], 34344);
		Window window("My window", 1280, 720);
		while (client.Update() && !window.ShouldClose()) {
			window.Update();
		}
	}
	else {
		Scene scene("main.lua");
		while (true) {
			scene.Update();
		}
	}

	enet_deinitialize();

	return 0;
}
