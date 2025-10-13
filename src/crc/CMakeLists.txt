add_library(crc STATIC crc.c)

target_include_directories(crc PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(crc PUBLIC "./include")
target_link_libraries(crc PUBLIC common)
