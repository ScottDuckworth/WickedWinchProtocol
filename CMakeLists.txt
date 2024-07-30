cmake_minimum_required(VERSION 3.22)

project(
  WickedWinchProtocol
  VERSION 0.1
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(protobuf
  GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
  GIT_TAG        v27.2
  SOURCE_SUBDIR  cmake
  FIND_PACKAGE_ARGS NAMES protobuf
)
FetchContent_MakeAvailable(protobuf)

include(FindProtobuf)
find_package(Protobuf REQUIRED)

include_directories(
  ${Protobuf_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/cpp
  ${CMAKE_CURRENT_BINARY_DIR}
)

protobuf_generate_cpp(WICKED_WINCH_PROTO_SRCS WICKED_WINCH_PROTO_HDRS proto/PathExpression.proto)

add_library(WickedWinchProtocol
  ${WICKED_WINCH_PROTO_HDRS}
  ${WICKED_WINCH_PROTO_SRCS}
  cpp/postfix/PostfixExpression.h
  cpp/postfix/PostfixExpression.cc
  cpp/patheval/PathEval.h
  cpp/patheval/PathEval.cc
)

target_link_libraries(WickedWinchProtocol ${Protobuf_LIBRARY})
target_include_directories(WickedWinchProtocol PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/cpp/include)

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

  add_executable(PostfixExpression_test
    cpp/postfix/PostfixExpression_test.cc
  )
  target_link_libraries(PostfixExpression_test
    GTest::gmock
    GTest::gtest_main
    WickedWinchProtocol
  )
  gtest_discover_tests(PostfixExpression_test)

  add_executable(PathEval_test
    cpp/patheval/PathEval_test.cc
  )
  target_link_libraries(PathEval_test
    GTest::gmock
    GTest::gtest_main
    WickedWinchProtocol
  )
  gtest_discover_tests(PathEval_test)
endif()
