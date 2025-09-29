cmake_minimum_required(VERSION 3.16)
project(ChatServer)

# C++17 표준 설정
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Boost 경로 설정
set(BOOST_ROOT "C:/Work/Libs/boost_1_89_0")
set(Boost_INCLUDE_DIR "${BOOST_ROOT}")

# Boost 헤더 찾기
find_package(Boost REQUIRED)

# 실행 파일 생성
add_executable(chat_server chat_server.cpp)
add_executable(chat_client chat_client.cpp)

# 헤더 포함 경로
target_include_directories(chat_server PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(chat_client PRIVATE ${Boost_INCLUDE_DIRS})

# Windows용 링크 라이브러리
if(WIN32)
    target_link_libraries(chat_server ws2_32)
    target_link_libraries(chat_client ws2_32)
else()
    target_link_libraries(chat_server pthread)
    target_link_libraries(chat_client pthread)
endif()

# 컴파일 옵션
if(MSVC)
    target_compile_options(chat_server PRIVATE /W4)
    target_compile_options(chat_client PRIVATE /W4)
else()
    target_compile_options(chat_server PRIVATE -Wall -Wextra)
    target_compile_options(chat_client PRIVATE -Wall -Wextra)
endif()

# Release 빌드 기본 설정
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 출력 디렉토리 설정
set_target_properties(chat_server chat_client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)