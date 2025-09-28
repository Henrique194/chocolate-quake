add_library(host STATIC
    host.c
    host_cmd.c
)

target_include_directories(host PRIVATE ${CMAKE_BINARY_DIR} "../")
target_link_libraries(host PRIVATE
    ${SDL2_LIBRARIES}
    camera
    client
    cmd
    common
    console
    crc
    end_screen
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
