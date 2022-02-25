# Copyright 2022 Justus Zorn

cmake_minimum_required(VERSION 3.21)

project("Hazard")

add_subdirectory("3rdParty/lua")

file(GLOB HazardSourceFiles "Source/*.cpp")

add_executable("Hazard" ${HazardSourceFiles})
target_link_libraries("Hazard" PRIVATE "lua")
