add_library(cmd STATIC cmd.c)

target_include_directories(cmd PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(cmd PUBLIC "./include")
target_link_libraries(cmd PRIVATE
    ${SDL2_LIBRARIES}
    camera
    client
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
