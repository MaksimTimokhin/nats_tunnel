file(GLOB SRC_CPP *.cpp config/*.cpp redirector/*.cpp)
add_library(tunnel ${SRC_CPP})
add_proto_library(connector Connector.proto)
target_link_libraries(tunnel nats_cpp connector nats_rpc)