add_library(memory STATIC zone.c)

target_include_directories(memory PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(memory PUBLIC "./include")
target_link_libraries(memory PRIVATE
    ${SDL2_LIBRARIES}
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
