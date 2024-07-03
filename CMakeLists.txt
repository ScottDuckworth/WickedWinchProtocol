cmake_minimum_required(VERSION 3.22)

project(
  WickedWinchProtocol
  VERSION 0.1
  LANGUAGES CXX)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(cpp/postfix)
add_subdirectory(proto)
