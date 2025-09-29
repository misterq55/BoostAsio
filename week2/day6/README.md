# Week2 Day6: Chat Server 빌드 가이드

## 빌드 방법

### 1. CMake 사용 (권장)
```bash
# 자동 빌드 스크립트 실행
build.bat

# 또는 수동 빌드
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### 2. 직접 컴파일
```bash
# MSVC - 서버
cl /EHsc /std:c++17 /I "C:\Work\Libs\boost_1_89_0" chat_server.cpp /Fe:chat_server.exe

# MSVC - 클라이언트
cl /EHsc /std:c++17 /I "C:\Work\Libs\boost_1_89_0" chat_client.cpp /Fe:chat_client.exe

# g++ (MinGW) - 서버
g++ -std=c++17 -I "C:\Work\Libs\boost_1_89_0" chat_server.cpp -o chat_server.exe -lws2_32

# g++ (MinGW) - 클라이언트
g++ -std=c++17 -I "C:\Work\Libs\boost_1_89_0" chat_client.cpp -o chat_client.exe -lws2_32
```

## 실행 방법

### 서버 실행
```bash
# 포트 9999에서 서버 시작
chat_server.exe 9999

# 여러 포트에서 동시 실행
chat_server.exe 9999 10000 10001
```

### 클라이언트 실행
```bash
# 로컬 서버에 접속
chat_client.exe localhost 9999

# 원격 서버에 접속
chat_client.exe 192.168.1.100 9999
```

## 테스트 방법

### 방법 1: Boost 클라이언트 사용 (권장)
1. 터미널 1: `chat_server.exe 9999`
2. 터미널 2: `chat_client.exe localhost 9999`
3. 터미널 3: `chat_client.exe localhost 9999` (다중 클라이언트)
4. 각 클라이언트에서 메시지 입력 → 모든 클라이언트에게 브로드캐스트

### 방법 2: Telnet 사용
1. 서버 실행: `chat_server.exe 9999`
2. Telnet으로 접속: `telnet localhost 9999`
3. 메시지 전송: `0012Hello World!` (4자리 길이 + 메시지)

## 파일 구조

- `chat_server.cpp`: 멀티 클라이언트 채팅 서버 (Boost 공식 예제)
- `chat_client.cpp`: 채팅 클라이언트 (Boost 공식 예제)
- `chat_message.hpp`: 메시지 프로토콜 정의 (Boost 공식 예제)
- `CMakeLists.txt`: CMake 빌드 설정
- `build.bat`: Windows 자동 빌드 스크립트
- `analysis.md`: 코드 구조 분석 문서

## chat_client.cpp 특징

### 핵심 패턴
- **멀티스레딩**: `std::thread`로 네트워크 I/O와 사용자 입력 분리
- **비동기 읽기/쓰기**: 서버와 동일한 패턴
- **post() 사용**: 스레드 안전한 메시지 전송

### 코드 구조
```cpp
main() {
    // 메인 스레드: 사용자 입력 처리
    while (std::cin.getline(line, ...)) {
        // 메시지 생성 및 전송
    }

    // 별도 스레드: 네트워크 I/O
    std::thread t([&io_context](){ io_context.run(); });
}
```