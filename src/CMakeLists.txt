set(CMAKE_BUILD_TYPE Release)

SET(CMAKE_C_FLAGS " ${CMAKE_C_FLAGS} -O3 -Wall -pedantic -std=gnu89")

cmake_minimum_required(VERSION 2.8)
project(assrender)
link_directories(${ASSRENDER_BINARY_DIR})
add_library(assrender SHARED assrender.c)
target_link_libraries(assrender -s -Wl,--gc-sections ass enca fontconfig expat harfbuzz fribidi glib-2.0 intl freetype iconv z avisynth.lib)
