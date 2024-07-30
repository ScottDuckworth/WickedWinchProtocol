include(FetchContent)

include(FindProtobuf)
include_directories(${Protobuf_INCLUDE_DIRS})
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS PathExpression.proto)
add_library(proto ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/..)
target_link_libraries(proto ${Protobuf_LIBRARIES})
