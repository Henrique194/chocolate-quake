set(LIB sys)

add_library(${LIB} STATIC src/sys.c)

target_include_directories(${LIB} PRIVATE ${CMAKE_BINARY_DIR} "../")
target_include_directories(${LIB} PUBLIC "./include")
target_link_libraries(${LIB} PUBLIC common host)
target_link_libraries(${LIB} PRIVATE
    ${SDL2_LIBRARIES}
    client
    cmd
    end_screen
    input
    menu
)