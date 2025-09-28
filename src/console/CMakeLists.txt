add_library(console STATIC
    console.c
    cvar.c
)

target_include_directories(console PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(console PUBLIC "./include")
target_link_libraries(console PRIVATE
    ${SDL2_LIBRARIES}
    camera
    client
    cmd
    common
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
