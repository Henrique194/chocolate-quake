add_subdirectory("end_screen")
add_subdirectory("input")
add_subdirectory("video")

set(SOURCE_FILES
    adivtab.h
    anorms.h
    bgmusic.c
    bgmusic.h
    bspfile.h
    chase.c
    cl_demo.c
    cl_input.c
    cl_main.c
    cl_parse.c
    cl_tent.c
    client.h
    cmd.c
    cmd.h
    common.c
    common.h
    console.c
    console.h
    crc.c
    crc.h
    cvar.c
    cvar.h
    d_edge.c
    d_fill.c
    d_iface.h
    d_init.c
    d_local.h
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
    draw.h
    host.c
    host_cmd.c
    keys.c
    keys.h
    mathlib.c
    mathlib.h
    menu.c
    menu.h
    model.c
    model.h
    modelgen.h
    net.h
    net_loop.c
    net_loop.h
    net_main.c
    net_none.c
    net_vcr.c
    net_vcr.h
    nonintel.c
    pr_cmds.c
    pr_comp.h
    pr_edict.c
    pr_exec.c
    progdefs.h
    progs.h
    protocol.h
    quakedef.h
    r_aclip.c
    r_alias.c
    r_bsp.c
    r_draw.c
    r_edge.c
    r_efrag.c
    r_light.c
    r_local.h
    r_main.c
    r_misc.c
    r_part.c
    r_shared.h
    r_sky.c
    r_sprite.c
    r_surf.c
    r_vars.c
    render.h
    sbar.c
    sbar.h
    screen.c
    screen.h
    server.h
    snd_codec.c
    snd_codec.h
    snd_dma.c
    snd_mem.c
    snd_mix.c
    snd_sdl.c
    snd_vorbis.c
    snd_vorbis.h
    sound.h
    spritegn.h
    sv_main.c
    sv_move.c
    sv_phys.c
    sv_user.c
    sys.h
    sys_sdl.c
    view.c
    view.h
    wad.c
    wad.h
    world.c
    world.h
    zone.c
    zone.h
)

if(WIN32)
    string(CONCAT RC_VERSION
        "${PROJECT_VERSION_MAJOR}, "
        "${PROJECT_VERSION_MINOR}, "
        "${PROJECT_VERSION_PATCH}, "
        "0"
    )
    set(WIN_DIR "${CMAKE_SOURCE_DIR}/dist/win")
    set(RC_NAME "quake.rc")
    set(RC_INPUT_FILE "${WIN_DIR}/${RC_NAME}.in")
    set(RC_OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/${RC_NAME}")

    configure_file(${RC_INPUT_FILE} ${RC_OUTPUT_FILE})
    add_executable(${PACKAGE_TARNAME} WIN32 ${SOURCE_FILES} ${RC_OUTPUT_FILE})
else()
    add_executable(${PACKAGE_TARNAME} ${SOURCE_FILES})
endif()

target_include_directories(${PACKAGE_TARNAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
)

set(LIBS ${SDL2_LIBRARIES} end_screen input video)
if(UNIX AND NOT APPLE)
    list(APPEND LIBS ${VORBIS_LIBRARY} ${VORBISFILE_LIBRARY})
else()
    list(APPEND LIBS Vorbis::vorbisfile)
endif()
target_link_libraries("${PACKAGE_TARNAME}" ${LIBS})

if(MSVC)
    set_target_properties(${PACKAGE_TARNAME} PROPERTIES
        LINK_FLAGS
        "/MANIFEST:NO"
    )
endif()
