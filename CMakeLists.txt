cmake_minimum_required(VERSION 3.14)
project(NatsTunnel)

set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_FLAGS "-Wall -Werror")
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FindProtobuf)
include(FindProtoLibrary)

find_package(Poco QUIET COMPONENTS Net JSON)

if (NOT Poco_FOUND)
    message(FATAL_ERROR "POCO not found")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(nats)
add_subdirectory(tunnel)

add_executable(NatsTunnel main.cpp)
target_link_libraries(NatsTunnel tunnel)

if (TESTING)
    add_subdirectory(tests)
endif()