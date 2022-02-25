// Copyright 2022 Justus Zorn

#include "Scene.h"
#include "Window.h"

using namespace Hazard;

int main(int argc, char* argv[]) {
	Scene scene("main.lua");
	Window window("My window", 1280, 720);

	while (!window.ShouldClose()) {
		window.Update();
	}

	return 0;
}
