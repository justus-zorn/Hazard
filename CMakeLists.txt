# Copyright 2022 Justus Zorn

cmake_minimum_required(VERSION 3.21)

project("Hazard")

add_subdirectory("3rdParty/lua")
add_subdirectory("3rdParty/enet")
add_subdirectory("3rdParty/SDL2")
add_subdirectory("3rdParty/portaudio")
add_subdirectory("3rdParty/stb")
add_subdirectory("3rdParty/glad")

file(GLOB HazardSourceFiles "Source/*.cpp")

add_executable("Hazard" ${HazardSourceFiles})
target_link_libraries("Hazard" PRIVATE "lua" "enet" "SDL2::SDL2-static" "SDL2::SDL2main" "portaudio_static" "stb" "glad")
