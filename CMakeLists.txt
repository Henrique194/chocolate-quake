cmake_minimum_required(VERSION 3.21.0)
project("Chocolate Quake" VERSION 1.3.0 LANGUAGES C)
set(CMAKE_C_STANDARD 99)

set(PACKAGE_NAME ${PROJECT_NAME})
set(PACKAGE_TARNAME "chocolate-quake")
set(PACKAGE_VERSION ${PROJECT_VERSION})
set(PACKAGE_STRING "${PROJECT_NAME} ${PROJECT_VERSION}")
set(PACKAGE_COPYRIGHT "Copyright (C) 1996-2025")
set(PACKAGE_LICENSE "GNU General Public License, version 3")
set(APPSTREAM_APP_ID "io.github.henrique194.chocolate-quake")

# Find math library
find_library(MATH m)
if(MATH)
    link_libraries(${MATH})
endif()

find_package(SDL2 2.26.5 REQUIRED)
if(UNIX AND NOT APPLE)
	find_library(VORBIS_LIBRARY vorbis REQUIRED)
	find_library(VORBISFILE_LIBRARY vorbisfile REQUIRED)
	find_library(MAD_LIBRARY mad REQUIRED)
	find_library(FLAC_LIBRARY FLAC REQUIRED)
else()
	find_package(Vorbis CONFIG REQUIRED)
	find_package(mad CONFIG REQUIRED)
	find_package(FLAC CONFIG REQUIRED)
endif()

configure_file(config.h.in config.h)

add_subdirectory("src")

if (UNIX AND NOT APPLE)
	install(FILES ${CMAKE_BINARY_DIR}/src/chocolate-quake
		DESTINATION bin
		PERMISSIONS WORLD_READ WORLD_EXECUTE GROUP_READ GROUP_EXECUTE OWNER_READ OWNER_WRITE OWNER_EXECUTE)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/dist/linux/${APPSTREAM_APP_ID}.svg
		DESTINATION share/icons/hicolor/scalable/apps
		PERMISSIONS WORLD_READ GROUP_READ OWNER_READ OWNER_WRITE)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/dist/linux/${APPSTREAM_APP_ID}.desktop
		DESTINATION share/applications
		PERMISSIONS WORLD_READ GROUP_READ OWNER_READ OWNER_WRITE)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/dist/linux/${APPSTREAM_APP_ID}.metainfo.xml
		DESTINATION share/metainfo
		PERMISSIONS WORLD_READ GROUP_READ OWNER_READ OWNER_WRITE)
endif()
