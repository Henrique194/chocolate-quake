add_library(common STATIC common.c)

target_include_directories(common PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(common PUBLIC "./include")
target_link_libraries(common PRIVATE
    ${SDL2_LIBRARIES}
    camera
    client
    cmd
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
