add_library(common STATIC
    com_argv.c
    com_byte.c
    com_ext.c
    com_fs.c
    com_init.c
    com_link.c
    com_msg.c
    com_sizebuf.c
    com_stdio.c
    com_stdlib.c
    com_string.c
    com_token.c
    com_va.c
)

target_include_directories(common PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(common PUBLIC "./include")
target_link_libraries(common PUBLIC ${SDL2_LIBRARIES})
target_link_libraries(common PRIVATE
    cmd
    console
    crc
    memory
    net
    renderer
    sys
)
