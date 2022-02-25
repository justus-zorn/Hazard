# Copyright 2022 Justus Zorn

cmake_minimum_required(VERSION 3.21)

project("Hazard")

file(GLOB HazardSourceFiles "Source/*.cpp")

add_executable("Hazard" ${HazardSourceFiles})
