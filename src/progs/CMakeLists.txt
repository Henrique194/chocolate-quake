add_library(progs STATIC
    pr_cmds.c
    pr_edict.c
    pr_exec.c
)

target_include_directories(progs PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(progs PUBLIC "./include")
target_link_libraries(progs PRIVATE
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
    renderer
    screen
    server
    sound
    status_bar
    sys
    video
    wad
)
