add_library(ogg STATIC
    src/bitwise.c
    src/crctable.h
    src/framing.c
)

target_include_directories(ogg PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)