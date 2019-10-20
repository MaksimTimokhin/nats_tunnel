add_subdirectory(rpc_gen)

add_library(nats_rpc Rpc.cpp)

add_proto_library(nats_rpc_proto rpc.proto)

target_link_libraries(nats_rpc nats_cpp nats_rpc_proto)