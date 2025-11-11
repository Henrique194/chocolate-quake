add_library(net STATIC
    net_dgrm.c
    net_dgrm.h
    net_drivers.c
    net_loop.c
    net_loop.h
    net_main.c
    net_poll.c
    net_poll.h
    net_socket.c
    net_socket.h
    net_udp.c
    net_udp.h
    net_vcr.c
    net_vcr.h
)

target_include_directories(net PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(net PUBLIC "./include")
target_link_libraries(net PUBLIC
    $<IF:$<TARGET_EXISTS:SDL2_net::SDL2_net>,SDL2_net::SDL2_net,SDL2_net::SDL2_net-static>
    common
    console
)
target_link_libraries(net PRIVATE
    client
    cmd
    host
    input
    memory
    screen
    server
    sys
)
