cmake_minimum_required(VERSION 3.22)

project(
  WickedWinchProtocol
  VERSION 0.1
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/cpp/include
  ${CMAKE_CURRENT_BINARY_DIR}
)

add_library(WickedWinchProtocol
  cpp/include/WickedWinchProtocol.h
  cpp/include/WickedWinchProtocol/EvalStatus.h
  cpp/src/WickedWinchProtocol/EvalStatus.cc
  cpp/include/WickedWinchProtocol/Postfix.h
  cpp/src/WickedWinchProtocol/Postfix.cc
  cpp/include/WickedWinchProtocol/Path.h
  cpp/src/WickedWinchProtocol/Path.cc
  cpp/include/WickedWinchProtocol/upper_bound.hh
)

target_include_directories(WickedWinchProtocol PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/cpp/include
)

if(NOT WICKEDWINCHPROTOCOL_TESTING_DISABLED)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  )
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
  enable_testing()
  include(GoogleTest)

  add_executable(Postfix_test
    cpp/tests/Postfix_test.cc
  )
  target_link_libraries(Postfix_test
    GTest::gmock
    GTest::gtest_main
    WickedWinchProtocol
  )
  gtest_discover_tests(Postfix_test)

  add_executable(Path_test
    cpp/tests/Path_test.cc
  )
  target_link_libraries(Path_test
    GTest::gmock
    GTest::gtest_main
    WickedWinchProtocol
  )
  gtest_discover_tests(Path_test)
endif()
