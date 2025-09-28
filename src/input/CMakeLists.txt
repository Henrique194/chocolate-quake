add_library(input STATIC
    in_gamepad.c
    in_gamepad.h
    in_keyboard.c
    in_keyboard.h
    in_main.c
    in_mouse.c
    in_mouse.h
    keys.c
)

target_include_directories(input PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(input PUBLIC "./include")
target_link_libraries(input PRIVATE
    ${SDL2_LIBRARIES}
    camera
    client
    cmd
    common
    console
    crc
    end_screen
    host
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
