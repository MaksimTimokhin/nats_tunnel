find_package(GTest REQUIRED)

file(GLOB TEST_SRC_FILES "*.cpp")

add_executable(tests ${TEST_SRC_FILES})

add_proto_library(test_rpc_proto rpc_test.proto)

target_include_directories(tests PUBLIC ${GTEST_INCLUDE_DIRS})
target_link_libraries(tests nats_cpp nats_rpc test_rpc_proto ${GTEST_LIBRARIES})