add_library(host STATIC
    host.c
    host_cmd.c
)

target_include_directories(host PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(host PUBLIC "./include")
target_link_libraries(host PUBLIC common console)
target_link_libraries(host PRIVATE
    ${SDL2_LIBRARIES}
    camera
    client
    cmd
    input
    menu
    model
    progs
    renderer
    screen
    server
    sound
    sys
    status_bar
    wad
)
