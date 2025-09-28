add_library(mathlib STATIC mathlib.c)

target_include_directories(mathlib PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(mathlib PUBLIC "./include")
target_link_libraries(mathlib PRIVATE
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
