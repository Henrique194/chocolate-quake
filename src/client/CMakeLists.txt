add_library(client STATIC
    cl_demo.c
    cl_input.c
    cl_main.c
    cl_parse.c
    cl_tent.c
)

target_include_directories(client PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(client PUBLIC "./include")
target_link_libraries(client PUBLIC
    common
    console
    mathlib
    net
    renderer
    video
)
target_link_libraries(client PRIVATE
    camera
    cmd
    host
    input
    model
    screen
    server
    sound
    status_bar
    sys
)
