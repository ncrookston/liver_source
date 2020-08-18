
add_executable(distribute_test distribute_tree_test.cpp)
target_link_libraries(distribute_test PRIVATE messages LiverLib ${CONAN_LIBS})

add_test(NAME distribute_tester COMMAND distribute_test WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

