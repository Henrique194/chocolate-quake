add_library(memory STATIC zone.c)

target_include_directories(memory PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(memory PUBLIC "./include")
target_link_libraries(memory PUBLIC common)
target_link_libraries(memory PRIVATE
    ${SDL2_LIBRARIES}
    cmd
    console
    sys
)
