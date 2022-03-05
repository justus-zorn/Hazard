# Hazard Game Engine
Hazard is a simple 2D game engine intended for developing small multiplayer games.

Take a look at the [Documentation](DOCS.md) for more information.

## Getting started
The easiest way to get started is to download a binary executable for Windows in the Releases
section. To build Hazard yourself, CMake version 3.21 or higher and a C++ compiler are required.

First, you have to clone the project from GitHub:

`git clone https://github.com/justus-zorn/Hazard`

After switching to the project directory, you can generate the project using CMake:

`cmake -B out`

Depending on which tool you want to use to generate the project, you might need to select a
generator using `-G <generator>`. For more information, take a look at the CMake documentation.

After generation is finished, the project can be built using CMake:

`cmake --build out`

To build Hazard in release mode, add `--config Release`.

## Dependencies
Hazard depends on enet for networking, Lua for scripting, SDL for rendering, SDL_image for
loading images, and SDL_ttf as well as Freetype for rendering text. SDL_ttf is very slightly
modified to use the included Freetype library.
