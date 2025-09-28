add_library(server STATIC
    sv_main.c
    sv_move.c
    sv_phys.c
    sv_user.c
    world.c
)

target_include_directories(server PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(server PUBLIC "./include")
target_link_libraries(server PRIVATE
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
    sound
    status_bar
    sys
    video
    wad
)
