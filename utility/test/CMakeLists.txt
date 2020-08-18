add_executable(bresenham_test bresenham_test.cpp)
target_link_libraries(bresenham_test PRIVATE messages LiverLib ${CONAN_LIBS})
add_test(NAME bresenham_tester COMMAND bresenham_test)

add_executable(interp_test interp_test.cpp)
target_link_libraries(interp_test  PRIVATE messages LiverLib ${CONAN_LIBS})
add_test(NAME interp_tester COMMAND interp_test)

add_executable(make_balanced_sampler_test make_balanced_sampler_test.cpp)
target_link_libraries(make_balanced_sampler_test PRIVATE messages LiverLib ${CONAN_LIBS})
add_test(NAME make_balanced_sampler_tester COMMAND make_balanced_sampler_test)

add_executable(meta_test meta_test.cpp)
target_link_libraries(meta_test PRIVATE messages LiverLib ${CONAN_LIBS})
add_test(NAME meta_tester COMMAND meta_test)

add_executable(octtree_test octtree_test.cpp)
target_link_libraries(octtree_test PRIVATE messages LiverLib ${CONAN_LIBS})
add_test(NAME octtree_tester COMMAND octtree_test)

add_executable(tree_test tree_test.cpp)
target_link_libraries(tree_test PRIVATE messages LiverLib ${CONAN_LIBS})
add_test(NAME tree_tester COMMAND tree_test)

add_executable(volume_image_test volume_image_test.cpp)
target_link_libraries(volume_image_test PRIVATE messages LiverLib ${CONAN_LIBS})
add_test(NAME volume_image_tester COMMAND volume_image_test)

add_executable(options_test options_test.cpp)
target_link_libraries(options_test PRIVATE messages LiverLib ${CONAN_LIBS})
add_test(NAME options_tester COMMAND options_test)

