cmake_minimum_required(VERSION 3.10)
project(Day5_UDP_Echo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Boost 라이브러리 경로 설정 (헤더 전용)
set(BOOST_ROOT "C:/Work/Libs/boost_1_89_0")
set(Boost_INCLUDE_DIRS "${BOOST_ROOT}")

# 공통 컴파일 옵션
if(MSVC)
    add_compile_options(/EHsc)
    add_compile_definitions(_WIN32_WINNT=0x0601)
endif()

# 서버 실행파일
add_executable(async_udp_echo_server async_udp_echo_server.cpp)
target_include_directories(async_udp_echo_server PRIVATE ${Boost_INCLUDE_DIRS})

# 클라이언트 실행파일
add_executable(blocking_udp_echo_client blocking_udp_echo_client.cpp)
target_include_directories(blocking_udp_echo_client PRIVATE ${Boost_INCLUDE_DIRS})

# Windows에서 추가 라이브러리 링크
if(WIN32)
    target_link_libraries(async_udp_echo_server ws2_32 wsock32)
    target_link_libraries(blocking_udp_echo_client ws2_32 wsock32)
endif()

# 빌드 디렉토리 설정
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)