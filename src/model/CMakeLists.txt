add_library(model STATIC model.c)

target_include_directories(model PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(model PUBLIC "./include")
target_link_libraries(model PUBLIC common mathlib memory renderer)
target_link_libraries(model PRIVATE console sys)
