add_library(screen STATIC screen.c)

target_include_directories(screen PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(screen PUBLIC "./include")
target_link_libraries(screen PUBLIC common console)
target_link_libraries(screen PRIVATE
    camera
    client
    cmd
    renderer
    host
    input
    menu
    status_bar
    sound
    sys
    video
    wad
)
