add_library(wad STATIC wad.c)

target_include_directories(wad PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(wad PUBLIC "./include")
target_link_libraries(wad PUBLIC common)
target_link_libraries(wad PRIVATE sys)
