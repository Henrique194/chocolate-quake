add_library(video STATIC
    vid_buffers.c
    vid_buffers.h
    vid_main.c
    vid_menu.c
    vid_menu.h
    vid_modes.c
    vid_modes.h
    vid_window.c
    vid_window.h
)

target_include_directories(video PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(video PUBLIC "./include")
target_link_libraries(video PRIVATE ${SDL2_LIBRARIES} input)
