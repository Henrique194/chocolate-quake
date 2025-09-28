add_library(menu STATIC menu.c)

target_include_directories(menu PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(menu PUBLIC "./include")
target_link_libraries(menu PRIVATE
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
