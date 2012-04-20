cmake_minimum_required(VERSION 2.8)

project(assrender)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBASS REQUIRED libass>=0.10.0)
find_package(EXPAT REQUIRED)
find_package(ZLIB REQUIRED)

if (CMAKE_COMPILER_IS_GNUCXX)
    add_definitions("-Wall -Wno-comment -pedantic -std=gnu99")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "-s")
    set(CMAKE_C_FLAGS_RELEASE "-O3")
endif()

add_subdirectory(src)
