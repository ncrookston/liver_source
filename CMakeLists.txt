cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
project(LiverModel VERSION 2.0.0 LANGUAGES CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

#set(BOOST_ROOT ../boost)
#find_package(Boost REQUIRED)
#find_package(fmt REQUIRED)
#set(Matlab_ROOT_DIR "/usr/local/MATLAB/R2018b")
#find_package(Matlab REQUIRED)

add_subdirectory(messages)

add_library(LiverLib INTERFACE)
target_compile_definitions(LiverLib INTERFACE GLM_ENABLE_EXPERIMENTAL)
target_include_directories(LiverLib INTERFACE . ${CMAKE_CURRENT_BINARY_DIR})
configure_file("utility/git_hash.cpp.in" "${CMAKE_BINARY_DIR}/git_hash.cpp" @ONLY)
target_sources(LiverLib INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/git_hash.cpp)

# artery tree
#add_executable(adjust_radii artery_tree/adjust_radii.cpp)
#target_link_libraries(adjust_radii PRIVATE messages LiverLib ${CONAN_LIBS})

add_executable(build_artery_tree artery_tree/build_artery_tree.cpp)
target_link_libraries(build_artery_tree PRIVATE messages LiverLib ${CONAN_LIBS})

add_executable(characterize_tree artery_tree/characterize_tree.cpp)
target_link_libraries(characterize_tree PRIVATE messages LiverLib ${CONAN_LIBS})

add_executable(view_artery_tree artery_tree/view_artery_tree.cpp)
target_link_libraries(view_artery_tree PRIVATE messages LiverLib ${CONAN_LIBS})

# distribution

message(STATUS ${Matlab_INCLUDE_DIRS})
add_executable(distribute_walrand distribution/distribute_walrand.cpp)
target_link_libraries(distribute_walrand PRIVATE messages LiverLib ${CONAN_LIBS})

enable_testing()
add_subdirectory(utility/test)
add_subdirectory(artery_tree/test)
add_subdirectory(distribution/test)
