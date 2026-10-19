cmake_minimum_required(VERSION 3.16)
project(SuperJupiterSampleWindow VERSION 0.1.0 LANGUAGES CXX)

set(_superjupiter_sample_window_top_level OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(_superjupiter_sample_window_top_level ON)
endif()
option(SUPERJUPITER_SAMPLE_WINDOW_BUILD_TESTS "Build SampleWindow tests" ${_superjupiter_sample_window_top_level})
option(SUPERJUPITER_SAMPLE_WINDOW_BUILD_EXAMPLES "Build SampleWindow host example" ${_superjupiter_sample_window_top_level})
option(SUPERJUPITER_SAMPLE_WINDOW_INSTALL "Generate SampleWindow install rules" ${_superjupiter_sample_window_top_level})

add_library(SuperJupiterSampleWindow INTERFACE)
add_library(SuperJupiter::SampleWindow ALIAS SuperJupiterSampleWindow)
set_target_properties(SuperJupiterSampleWindow PROPERTIES EXPORT_NAME SampleWindow)
target_include_directories(SuperJupiterSampleWindow INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:include>)
target_compile_features(SuperJupiterSampleWindow INTERFACE cxx_std_11)

if(SUPERJUPITER_SAMPLE_WINDOW_BUILD_TESTS)
  enable_testing()
  add_executable(superjupiter-sample-window-tests tests/test_sample_window.cpp)
  target_link_libraries(superjupiter-sample-window-tests PRIVATE SuperJupiter::SampleWindow)
  add_test(NAME superjupiter-sample-window-tests COMMAND superjupiter-sample-window-tests)
  add_executable(superjupiter-sample-window-namespace-test tests/namespace_isolation.cpp)
  target_link_libraries(superjupiter-sample-window-namespace-test PRIVATE SuperJupiter::SampleWindow)
  add_test(NAME superjupiter-sample-window-namespace-test COMMAND superjupiter-sample-window-namespace-test)
endif()
if(SUPERJUPITER_SAMPLE_WINDOW_BUILD_EXAMPLES)
  add_executable(superjupiter-sample-window-replay tools/replay.cpp)
  target_link_libraries(superjupiter-sample-window-replay PRIVATE SuperJupiter::SampleWindow)
endif()
if(SUPERJUPITER_SAMPLE_WINDOW_INSTALL)
  include(CMakePackageConfigHelpers)
  set(_superjupiter_sample_window_package_dir lib/cmake/SuperJupiterSampleWindow)
  install(TARGETS SuperJupiterSampleWindow EXPORT SuperJupiterSampleWindowTargets)
  install(FILES src/SuperJupiterSampleWindow.h DESTINATION include)
  install(EXPORT SuperJupiterSampleWindowTargets NAMESPACE SuperJupiter::
    DESTINATION ${_superjupiter_sample_window_package_dir})
  configure_package_config_file(cmake/SuperJupiterSampleWindowConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/SuperJupiterSampleWindowConfig.cmake
    INSTALL_DESTINATION ${_superjupiter_sample_window_package_dir})
  write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/SuperJupiterSampleWindowConfigVersion.cmake
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMinorVersion ARCH_INDEPENDENT)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/SuperJupiterSampleWindowConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/SuperJupiterSampleWindowConfigVersion.cmake
    DESTINATION ${_superjupiter_sample_window_package_dir})
  install(FILES LICENSE DESTINATION share/licenses/SuperJupiterSampleWindow)
endif()
