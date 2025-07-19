cmake_minimum_required(VERSION 3.7.2)
project("Chocolate Quake" VERSION 1.0.0 LANGUAGES C)
set(CMAKE_C_STANDARD 99)

set(PACKAGE_NAME ${PROJECT_NAME})
set(PACKAGE_TARNAME "chocolate-quake")
set(PACKAGE_VERSION ${PROJECT_VERSION})
set(PACKAGE_STRING "${PROJECT_NAME} ${PROJECT_VERSION}")
set(PACKAGE_COPYRIGHT "Copyright (C) 1996-2025")
set(PACKAGE_LICENSE "GNU General Public License, version 3")

# Find math library
find_library(MATH m)
if(MATH)
    link_libraries(${MATH})
endif()

find_package(SDL2 2.26.5 REQUIRED)
find_package(Vorbis CONFIG REQUIRED)

configure_file(config.h.in config.h)

add_subdirectory("src")
