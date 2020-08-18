
macro(set_extensions)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(EXT ".so")
  set(DEXT ".so")
else()
  set(EXT ".dylib")
  set(DEXT "-d.dylib")
endif()
endmacro()

function(dmip_setup_target target path_from_target)

set(src_root "${CMAKE_CURRENT_SOURCE_DIR}/${path_from_target}")

set(CMAKE_MODULE_PATH "${src_root}/scripts")

include(GetGitRevisionDescription)
git_describe(GIT_SHALONG --always --abbrev=40 --long --dirty)

configure_file("${CMAKE_SOURCE_DIR}/../utility/git_hash.cpp.in"
  "${CMAKE_BINARY_DIR}/git_hash.cpp" @ONLY)
target_sources(${target} PUBLIC ${CMAKE_BINARY_DIR}/git_hash.cpp)

set(BOOST_INCLUDEDIR "${src_root}/../boost")
set(BOOST_LIBRARYDIR "${src_root}/../boost/stage/lib")
set(BOOST_ROOT "${src_root}/../boost")
#find_package(OpenCL REQUIRED)
#if (NOT OPENCL_FOUND)
#  message(FATAL_ERROR "Cannot find OpenCL")
#endif()

set(TBB_ROOT_DIR "${src_root}/../tbb/tbb44_20160413oss")
#include(FindTBB)
#find_package(TBB REQUIRED)
#if (NOT TBB_FOUND)
#  message(FATAL_ERROR "Cannot find Threaded Building Blocks")
#endif()
#message("TBB version: ${TBB_LIBRARIES}")

set_extensions()
target_link_libraries(${target}
    optimized "${BOOST_LIBRARYDIR}/libboost_container-mt${EXT}"
    debug     "${BOOST_LIBRARYDIR}/libboost_container-mt-d${EXT}"
    optimized "${BOOST_LIBRARYDIR}/libboost_filesystem-mt${EXT}"
    debug     "${BOOST_LIBRARYDIR}/libboost_filesystem-mt-d${EXT}"
    optimized "${BOOST_LIBRARYDIR}/libboost_iostreams-mt${EXT}"
    debug     "${BOOST_LIBRARYDIR}/libboost_iostreams-mt-d${EXT}"
    optimized "${BOOST_LIBRARYDIR}/libboost_program_options-mt${EXT}"
    debug     "${BOOST_LIBRARYDIR}/libboost_program_options-mt-d${EXT}"
    optimized "${BOOST_LIBRARYDIR}/libboost_system-mt${EXT}"
    debug     "${BOOST_LIBRARYDIR}/libboost_system-mt-d${EXT}"
    optimized "${src_root}/../fmt/_bld_release/fmt/libfmt.a"
    debug     "${src_root}/../fmt/_bld_debug/fmt/libfmt.a"
    optimized "${TBB_ROOT_DIR}/build/lib/libtbb${EXT}"
    debug     "${TBB_ROOT_DIR}/build/lib/libtbb_debug${EXT}"
#    ${OpenCL_LIBRARIES}
    optimized "-lz -pthread"
    debug     "-lz -pthread"
)
target_include_directories(${target} PUBLIC
    ${BOOST_INCLUDEDIR}
    ${src_root}
    ${src_root}/../range-v3/include
    ${src_root}/../Catch/include
    ${src_root}/../fmt
    ${src_root}/../protobuf/src
    ${TBB_ROOT_DIR}/include
    ${src_root}/../glm
)

target_compile_features(${target} PUBLIC cxx_variable_templates
    cxx_uniform_initialization cxx_static_assert cxx_raw_string_literals
    cxx_constexpr cxx_auto_type cxx_decltype_incomplete_return_types
    cxx_inheriting_constructors cxx_inline_namespaces cxx_lambdas
)

target_compile_definitions(${target} PRIVATE RANGES_SUPPRESS_IOTA_WARNING)

endfunction()

function (enable_opengl target path_from_target)

set(src_root "${CMAKE_CURRENT_SOURCE_DIR}/${path_from_target}")

find_package(OpenGL REQUIRED)
if (NOT OPENGL_FOUND)
  message(FATAL_ERROR "Cannot find OpenGL")
endif()

set_extensions()
target_link_libraries(${target}
    ${OPENGL_LIBRARIES}
    ${src_root}/../glew/lib/libGLEW${EXT}
    optimized "${src_root}/../SFML/_bld_release/lib/libsfml-audio${EXT}"
    optimized "${src_root}/../SFML/_bld_release/lib/libsfml-graphics${EXT}"
    optimized "${src_root}/../SFML/_bld_release/lib/libsfml-network${EXT}"
    optimized "${src_root}/../SFML/_bld_release/lib/libsfml-system${EXT}"
    optimized "${src_root}/../SFML/_bld_release/lib/libsfml-window${EXT}"
    debug     "${src_root}/../SFML/_bld_debug/lib/libsfml-audio${DEXT}"
    debug     "${src_root}/../SFML/_bld_debug/lib/libsfml-graphics${DEXT}"
    debug     "${src_root}/../SFML/_bld_debug/lib/libsfml-network${DEXT}"
    debug     "${src_root}/../SFML/_bld_debug/lib/libsfml-system${DEXT}"
    debug     "${src_root}/../SFML/_bld_debug/lib/libsfml-window${DEXT}"
)
target_include_directories(${target} PUBLIC
    ${src_root}/../SFML/include
    ${src_root}/../glew/include
)
target_compile_definitions(${target} PRIVATE GLM_ENABLE_EXPERIMENTAL)

endfunction()

function (enable_protobuf target path_to_target)
  set_extensions()
  set(src_root "${CMAKE_CURRENT_SOURCE_DIR}/${path_to_target}")
  include(FindProtobuf)
  set(protobuf_dir                  ${src_root}/../../protobuf/bin)
  set(Protobuf_INCLUDE_DIR          ${protobuf_dir}/include)
  set(Protobuf_LIBRARY              ${protobuf_dir}/lib/libprotobuf.a)
  set(Protobuf_LIBRARY_DEBUG        ${protobuf_dir}/lib/libprotobuf.a)
  set(Protobuf_LIBRARIES            "${protobuf_dir}/lib/libprotobuf.a;-lpthread;-lz")
  set(Protobuf_LITE_LIBRARY         ${protobuf_dir}/lib/libprotobuf-lite.a)
  set(Protobuf_LITE_LIBRARY_DEBUG   ${protobuf_dir}/lib/libprotobuf-lite.a)
  set(Protobuf_PROTOC_LIBRARY       ${protobuf_dir}/lib/libprotoc.a)
  set(Protobuf_PROTOC_LIBRARY_DEBUG ${protobuf_dir}/lib/libprotoc.a)
  set(Protobuf_PROTOC_EXECUTABLE    ${protobuf_dir}/bin/protoc)
  find_package(Protobuf "3.3.0" QUIET REQUIRED)
  if (NOT Protobuf_FOUND)
    message(FATAL_ERROR "Cannot find google protocol buffers")
  endif()
  target_include_directories(${target} PUBLIC
      ${CMAKE_CURRENT_BINARY_DIR}/${path_to_target}/protobuf
      ${Protobuf_INCLUDE_DIRS})
  target_link_libraries(${target} messages ${Protobuf_LIBRARIES})

  if (NOT ";${ARGN};" MATCHES "skip_subdirectory")
    add_subdirectory(${CMAKE_SOURCE_DIR}/../messages
                     ${CMAKE_CURRENT_BINARY_DIR}/protobuf/messages)
  endif()
endfunction()
