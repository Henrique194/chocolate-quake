add_library(menu STATIC menu.c)

target_include_directories(menu PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(menu PUBLIC "./include")
target_link_libraries(menu PUBLIC common)
target_link_libraries(menu PRIVATE
    ${SDL2_LIBRARIES}
    camera
    client
    cmd
    console
    host
    input
    renderer
    screen
    server
    sound
    video
)
