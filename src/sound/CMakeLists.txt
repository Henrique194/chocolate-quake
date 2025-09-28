add_library(sound STATIC
    bgmusic.c
    snd_codec.c
    snd_codec.h
    snd_dma.c
    snd_mem.c
    snd_mix.c
    snd_sdl.c
    snd_vorbis.c
    snd_vorbis.h
)

target_include_directories(sound PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(sound PUBLIC "./include")
target_link_libraries(sound PRIVATE
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
    status_bar
    sys
    video
    wad
)
