add_library(client STATIC
    cl_demo.c
    cl_input.c
    cl_main.c
    cl_parse.c
    cl_tent.c
)

target_include_directories(client PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(client PUBLIC "./include")
target_link_libraries(client PRIVATE
    ${SDL2_LIBRARIES}
    camera
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
