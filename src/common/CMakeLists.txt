add_library(common STATIC common.c)

target_include_directories(common PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(common PUBLIC "./include")
target_link_libraries(common PRIVATE
    cmd
    console
    crc
    memory
    net
    renderer
    sys
)
