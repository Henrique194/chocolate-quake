add_library(model STATIC model.c)

target_include_directories(model PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(model PUBLIC "./include")
target_link_libraries(model PRIVATE
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
