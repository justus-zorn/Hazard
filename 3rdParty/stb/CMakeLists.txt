project("stb")

add_library("stb" STATIC "stb.c")
target_include_directories("stb" PUBLIC "include")
