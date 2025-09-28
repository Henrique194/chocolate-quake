add_library(screen STATIC screen.c)

target_include_directories(screen PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(screen PUBLIC "./include")
target_link_libraries(screen PRIVATE
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
    server
    sound
    status_bar
    sys
    video
    wad
)
