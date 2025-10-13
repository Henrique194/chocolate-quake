add_library(cmd STATIC cmd.c)

target_include_directories(cmd PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(cmd PUBLIC "./include")
target_link_libraries(cmd PUBLIC common)
target_link_libraries(cmd PRIVATE client console host sys memory)
