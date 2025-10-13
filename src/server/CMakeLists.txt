add_library(server STATIC
    sv_main.c
    sv_move.c
    sv_phys.c
    sv_user.c
    world.c
)

target_include_directories(server PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(server PUBLIC "./include")
target_link_libraries(server PUBLIC
    client
    common
    console
    mathlib
    model
    net
    progs
)
target_link_libraries(server PRIVATE
    camera
    cmd
    host
    input
    sound
    sys
)
