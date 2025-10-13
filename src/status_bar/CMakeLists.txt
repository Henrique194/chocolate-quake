add_library(status_bar STATIC sbar.c)

target_include_directories(status_bar PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(status_bar PUBLIC "./include")
target_link_libraries(status_bar PUBLIC common)
target_link_libraries(status_bar PRIVATE
    client
    cmd
    renderer
    screen
    server
    wad
)
