add_library(console STATIC
    console.c
    cvar.c
)

target_include_directories(console PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(console PUBLIC "./include")
target_link_libraries(console PUBLIC common)
target_link_libraries(console PRIVATE
    client
    cmd
    input
    renderer
    screen
    server
    sound
    sys
)
