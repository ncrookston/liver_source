cmake_minimum_required(VERSION 3.3)

include(${CMAKE_SOURCE_DIR}/../dmip_setup.cmake)
project(to_im)

add_executable(from_im from_im.cpp)
dmip_setup_target(from_im "..")
set(DMIP "${CMAKE_SOURCE_DIR}/../../dmip_projects/installs/${CMAKE_BUILD_TYPE}")
target_include_directories(from_im PUBLIC "${DMIP}/include")
target_link_libraries(from_im
  ${DMIP}/lib/libimgio.a
  ${DMIP}/lib/libmiputil.a
  ${DMIP}/lib/libim.a)

add_executable(to_im to_im.cpp)
dmip_setup_target(to_im "..")
set(DMIP "${CMAKE_SOURCE_DIR}/../../dmip_projects/installs/${CMAKE_BUILD_TYPE}")
target_include_directories(to_im PUBLIC "${DMIP}/include")
target_link_libraries(to_im
  ${DMIP}/lib/libimgio.a
  ${DMIP}/lib/libmiputil.a
  ${DMIP}/lib/libim.a)

add_executable(zlib_exe zlib_exe.cpp)
dmip_setup_target(zlib_exe "..")
