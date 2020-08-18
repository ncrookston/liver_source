cmake_minimum_required(VERSION 3.10)

#include(${CMAKE_SOURCE_DIR}/../dmip_setup.cmake)
project(ArteryTree VERSION 2.0.0 LANGUAGES CXX)

#include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)

add_subdirectory(../liver artery_tree_liver)
add_executable(build_artery_tree build_artery_tree.cpp)

#dmip_setup_target(build_artery_tree "..")
#enable_protobuf(build_artery_tree ".")
#target_include_directories(build_artery_tree PRIVATE ${CMAKE_PROJECT_DIR}/liver)
target_compile_definitions(build_artery_tree PUBLIC GLM_ENABLE_EXPERIMENTAL)
target_link_libraries(build_artery_tree PRIVATE LiverLib)

#target_link_libraries(build_artery_tree ${CONAN_LIBS})

# add_executable(adjust_radii adjust_radii.cpp)
# dmip_setup_target(adjust_radii "..")
# enable_protobuf(adjust_radii "." "skip_subdirectory")
#
# add_executable(build_50_50_tree build_50_50_tree.cpp)
# dmip_setup_target(build_50_50_tree "..")
# enable_protobuf(build_50_50_tree "." "skip_subdirectory")
#
# add_executable(build_walrand_vat build_walrand_vat.cpp)
# dmip_setup_target(build_walrand_vat "..")
# enable_protobuf(build_walrand_vat "." "skip_subdirectory")
#
# add_executable(characterize_tree characterize_tree.cpp)
# dmip_setup_target(characterize_tree "..")
# enable_protobuf(characterize_tree "." "skip_subdirectory")
#
# add_executable(validate_tree validate_tree.cpp)
# dmip_setup_target(validate_tree "..")
# enable_protobuf(validate_tree "." "skip_subdirectory")
#
# add_executable(view_artery_tree view_artery_tree.cpp)
# dmip_setup_target(view_artery_tree "..")
# enable_opengl(view_artery_tree "..")
# enable_protobuf(view_artery_tree "." "skip_subdirectory")
#
# add_executable(make_vessel_tree_sequence make_vessel_tree_sequence.cpp)
# dmip_setup_target(make_vessel_tree_sequence "..")
# enable_opengl(make_vessel_tree_sequence "..")
# enable_protobuf(make_vessel_tree_sequence "." "skip_subdirectory")
#
# enable_testing()
# add_subdirectory(${CMAKE_SOURCE_DIR}/../utility/test
#                  ${CMAKE_CURRENT_BINARY_DIR}/utility_test)
#
# add_subdirectory(${CMAKE_SOURCE_DIR}/test
#   ${CMAKE_CURRENT_BINARY_DIR}/tree_load_test)
