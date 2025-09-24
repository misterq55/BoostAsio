cmake_minimum_required(VERSION 3.10)
project(Day3_TCP_Echo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BOOST_ROOT "C:/Work/Libs/boost_1_89_0")
include_directories(${BOOST_ROOT})

add_executable(blocking_tcp_echo_client blocking_tcp_echo_client.cpp)
add_executable(blocking_tcp_echo_server blocking_tcp_echo_server.cpp)

if(WIN32)
    target_link_libraries(blocking_tcp_echo_client ws2_32)
    target_link_libraries(blocking_tcp_echo_server ws2_32)
endif()