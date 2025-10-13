add_library(mathlib STATIC mathlib.c)

target_include_directories(mathlib PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(mathlib PUBLIC "./include")
target_link_libraries(mathlib PUBLIC common model)
