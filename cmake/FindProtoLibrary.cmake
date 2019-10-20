find_package(Protobuf REQUIRED)

function(add_proto_library target_name)
    list(REMOVE_AT ARGV 0)
    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${ARGV})
    add_library(${target_name} ${PROTO_SRCS} ${PROTO_HDRS})
    target_link_libraries(${target_name} PUBLIC nats_cpp ${Protobuf_LIBRARIES})
    target_include_directories(${target_name} PUBLIC ${CMAKE_BINARY_DIR})
endfunction()