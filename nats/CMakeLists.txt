find_library(NATS_LIB nats)
if (NOT NATS_LIB)
    message(FATAL_ERROR "nats client library not found")
endif ()

add_library(nats_cpp Nats.cpp)
target_link_libraries(nats_cpp ${NATS_LIB} ${Poco_LIBRARIES})
target_include_directories(nats_cpp PUBLIC ${Poco_INCLUDE_DIRS})
add_subdirectory(rpc)
