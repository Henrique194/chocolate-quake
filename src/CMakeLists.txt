set(INTERNAL_LIBS
    camera
    client
    cmd
    common
    console
    crc
    end_screen
    host
    input
    mathlib
    memory
    menu
    model
    net
    progs
    renderer
    screen
    server
    sound
    status_bar
    sys
    video
    wad
)
foreach(LIB ${INTERNAL_LIBS})
    add_subdirectory(${LIB})
endforeach()


set(EXTERNAL_LIBS ${SDL2_LIBRARIES})
if(UNIX AND NOT APPLE)
    list(APPEND EXTERNAL_LIBS ${VORBIS_LIBRARY} ${VORBISFILE_LIBRARY})
else()
    list(APPEND EXTERNAL_LIBS Vorbis::vorbisfile)
endif()


if(WIN32)
    string(CONCAT RC_VERSION
        "${PROJECT_VERSION_MAJOR}, "
        "${PROJECT_VERSION_MINOR}, "
        "${PROJECT_VERSION_PATCH}, "
        "0"
    )
    set(WIN_DIR "${CMAKE_SOURCE_DIR}/dist/win")
    set(RC_NAME "quake.rc")
    set(RC_INPUT_FILE "${WIN_DIR}/${RC_NAME}.in")
    set(RC_OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/${RC_NAME}")

    configure_file(${RC_INPUT_FILE} ${RC_OUTPUT_FILE})
    add_executable(${PACKAGE_TARNAME} WIN32 main.c ${RC_OUTPUT_FILE})
else()
    add_executable(${PACKAGE_TARNAME} main.c)
endif()

target_include_directories(${PACKAGE_TARNAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
)

target_link_libraries(${PACKAGE_TARNAME} ${INTERNAL_LIBS} ${EXTERNAL_LIBS})

if(MSVC)
    set_target_properties(${PACKAGE_TARNAME} PROPERTIES
        LINK_FLAGS
        "/MANIFEST:NO"
    )
endif()
