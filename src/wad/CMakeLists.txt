add_library(wad STATIC wad.c)

target_include_directories(wad PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(wad PUBLIC "./include")
target_link_libraries(wad PRIVATE
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
)
