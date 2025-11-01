add_library(net STATIC
    net_loop.c
    net_loop.h
    net_main.c
    net_none.c
    net_poll.c
    net_poll.h
    net_socket.c
    net_socket.h
    net_vcr.c
    net_vcr.h
)

target_include_directories(net PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(net PUBLIC "./include")
target_link_libraries(net PUBLIC common console)
target_link_libraries(net PRIVATE
    client
    cmd
    host
    memory
    server
    sys
)
