add_library(sys STATIC sys.c)

target_include_directories(sys PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(sys PUBLIC "./include")
target_link_libraries(sys PUBLIC common host)
target_link_libraries(sys PRIVATE
    ${SDL2_LIBRARIES}
    client
    cmd
    end_screen
    input
    menu
)
