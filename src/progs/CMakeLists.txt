add_library(progs STATIC
    pr_cmds.c
    pr_edict.c
    pr_exec.c
)

target_include_directories(progs PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(progs PUBLIC "./include")
target_link_libraries(progs PUBLIC common mathlib)
target_link_libraries(progs PRIVATE
    cmd
    console
    crc
    host
    server
    sys
)
