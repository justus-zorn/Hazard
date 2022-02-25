# Copyright 2022 Justus Zorn

cmake_minimum_required(VERSION 3.21)

project("Hazard")

add_subdirectory("3rdParty/lua")
add_subdirectory("3rdParty/SDL2")

file(GLOB HazardSourceFiles "Source/*.cpp")

add_executable("Hazard" ${HazardSourceFiles})
target_link_libraries("Hazard" PRIVATE "lua" "SDL2::SDL2-static" "SDL2::SDL2main")
