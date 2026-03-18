cmake_minimum_required(VERSION 3.10.0)
project(SocketLib VERSION 0.1.0 LANGUAGES C CXX)

include_directories(
    include/
)

set(SOURCES
    src/SocketLib_internal.cpp
    src/tcp_server.cpp
    src/tcp_client.cpp
    src/udp_server.cpp
    src/udp_client.cpp
    src/SocketLib.cpp
    src/SocketLib_tests.cpp
)

set(HEADERS
    include/SocketLib_internal.hpp
    include/SocketLib.hpp
)

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(SocketLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(SocketLib PUBLIC cxx_std_17)

if (WIN32)
    target_link_libraries(SocketLib PRIVATE ws2_32)
endif()
