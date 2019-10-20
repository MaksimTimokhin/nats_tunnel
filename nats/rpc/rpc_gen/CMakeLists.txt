add_executable(protoc-gen-rpc main.cpp RpcGenerator.cpp)
find_package(Protobuf REQUIRED)
target_link_libraries(protoc-gen-rpc ${Protobuf_PROTOC_LIBRARIES} ${Protobuf_LIBRARIES})