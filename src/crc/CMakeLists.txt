add_library(crc STATIC crc.c)

target_include_directories(crc PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(crc PUBLIC "./include")
target_link_libraries(crc PRIVATE
    ${SDL2_LIBRARIES}
    camera
    client
    cmd
    common
    console
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
