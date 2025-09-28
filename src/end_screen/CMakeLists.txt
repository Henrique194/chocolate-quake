add_library(end_screen STATIC
    fonts/large.h
    fonts/normal.h
    fonts/small.h
    es_buffer.c
    es_buffer.h
    es_font.c
    es_font.h
    es_main.c
    es_palette.c
    es_palette.h
    es_time.c
    es_time.h
    es_window.c
    es_window.h
)

target_include_directories(end_screen PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(end_screen PUBLIC "./include")
target_link_libraries(end_screen PRIVATE
    ${SDL2_LIBRARIES}
    camera
    client
    cmd
    common
    console
    crc
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
    sys
    video
    wad
)
