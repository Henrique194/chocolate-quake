add_library(camera STATIC
    chase.c
    view.c
)

target_include_directories(camera PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(camera PUBLIC "./include")
target_link_libraries(camera PRIVATE
    ${SDL2_LIBRARIES}
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
