add_executable(treediff treediff.cpp)
target_link_libraries(treediff PRIVATE messages LiverLib ${CONAN_LIBS})

add_executable(tree_load_test main.cpp)
target_link_libraries(tree_load_test PRIVATE messages LiverLib ${CONAN_LIBS})

add_test(NAME tree_load_tester COMMAND tree_load_test WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
