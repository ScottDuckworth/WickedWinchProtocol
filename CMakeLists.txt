cmake_minimum_required(VERSION 3.22)

project(
  WickedWinchProtocol
  VERSION 0.1
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

#FetchContent_Declare(protobuf
#  GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
#  GIT_TAG        v27.2
#  SOURCE_SUBDIR  cmake
#)
#FetchContent_MakeAvailable(protobuf)

if(NOT WICKEDWINCHPROTOCOL_TESTING_DISABLED)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  )
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
  enable_testing()
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/cpp)

add_subdirectory(cpp/postfix)
add_subdirectory(cpp/patheval)
add_subdirectory(proto)
