add_library(renderer STATIC
    adivtab.h
    anorms.h
    d_edge.c
    d_fill.c
    d_init.c
    d_modech.c
    d_part.c
    d_polyse.c
    d_scan.c
    d_sky.c
    d_sprite.c
    d_surf.c
    d_vars.c
    d_zpoint.c
    draw.c
    nonintel.c
    r_aclip.c
    r_alias.c
    r_bsp.c
    r_draw.c
    r_edge.c
    r_efrag.c
    r_light.c
    r_main.c
    r_misc.c
    r_part.c
    r_sky.c
    r_sprite.c
    r_surf.c
    r_vars.c
)

target_include_directories(renderer PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(renderer PUBLIC "./include")
target_link_libraries(renderer PUBLIC
    client
    common
    console
    mathlib
    model
    progs
    video
    wad
)
target_link_libraries(renderer PRIVATE
    camera
    cmd
    memory
    server
    screen
    sound
    status_bar
    sys
)
