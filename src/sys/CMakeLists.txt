add_library(sys STATIC sys.c)

target_include_directories(sys PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(sys PUBLIC "./include")
target_link_libraries(sys PRIVATE
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
    status_bar
    video
    wad
)
