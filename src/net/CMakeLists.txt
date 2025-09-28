add_library(net STATIC
    net_loop.c
    net_loop.h
    net_main.c
    net_none.c
    net_vcr.c
    net_vcr.h
)

target_include_directories(net PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(net PUBLIC "./include")
target_link_libraries(net PRIVATE
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
