include(FetchContent)

FetchContent_Declare(protobuf
  GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
  GIT_TAG        v27.2
  SOURCE_SUBDIR  cmake
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(protobuf)

find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS PathExpression.proto)
add_library(proto ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/..)
target_link_libraries(proto ${PROTOBUF_LIBRARY})
