cmake_minimum_required(VERSION 3.10)
project(BroadcastChatServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Boost 설정 (헤더 전용)
set(BOOST_ROOT "C:/Work/Libs/boost_1_89_0")
include_directories(${BOOST_ROOT})
include_directories(${CMAKE_SOURCE_DIR}/../day6)  # chat_message.hpp 경로

# 실습 서버
add_executable(broadcast_chat_server broadcast_chat_server.cpp)

if(WIN32)
    target_link_libraries(broadcast_chat_server ws2_32 wsock32)
endif()
