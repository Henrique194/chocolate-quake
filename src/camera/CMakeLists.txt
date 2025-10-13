add_library(camera STATIC chase.c view.c)

target_include_directories(camera PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(camera PUBLIC "./include")
target_link_libraries(camera PUBLIC common console mathlib)
target_link_libraries(camera PRIVATE
    client
    cmd
    host
    model
    renderer
    screen
    server
)
