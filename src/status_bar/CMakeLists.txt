add_library(status_bar STATIC sbar.c)

target_include_directories(status_bar PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(status_bar PUBLIC "./include")
target_link_libraries(status_bar PRIVATE
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
    sys
    video
    wad
)
