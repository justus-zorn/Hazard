# Copyright 2022 Justus Zorn

cmake_minimum_required(VERSION 3.21)

project("Hazard")

add_subdirectory("3rdParty/lua")
add_subdirectory("3rdParty/enet")
add_subdirectory("3rdParty/SDL2")
add_subdirectory("3rdParty/SDL_ttf")
add_subdirectory("3rdParty/portaudio")
add_subdirectory("3rdParty/stb")

file(GLOB HazardSourceFiles "Source/*.cpp")

add_executable("Hazard" ${HazardSourceFiles})
target_link_libraries("Hazard" PRIVATE "lua" "enet" "SDL2::SDL2-static" "SDL2::SDL2main" "SDL2_ttf" "portaudio_static" "stb")
target_include_directories("Hazard" PRIVATE "3rdParty/SDL_ttf")
