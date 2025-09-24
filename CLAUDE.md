# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 프로젝트 개요

Boost.Asio를 활용한 게임 서버 개발 학습 프로젝트입니다. 기본 타이머부터 고급 비동기 TCP/UDP 서버까지 단계별로 학습하며, 최종적으로 실시간 게임 서버 구현을 목표로 합니다.

## 빌드 및 컴파일

### C++ 컴파일러
- **MSVC (Visual Studio)**: `cl /EHsc /std:c++17 /I C:\Work\Libs\boost_1_89_0 <파일명>.cpp`
- **g++ (MinGW/GCC)**: `g++ -std=c++17 -I C:\Work\Libs\boost_1_89_0 <파일명>.cpp -o <출력명>`
- **Python**: `py` 명령어 사용 (python 아님)
- **Excel 파일 읽기**: openpyxl 사용 (pandas 아님)

### Boost 라이브러리 경로
- Boost 1.89.0 설치 경로: `C:\Work\Libs\boost_1_89_0`
- 헤더 전용 라이브러리로 링크 불필요 (Boost.Asio)
- Boost.System이 필요한 경우 `-lboost_system` 추가

## 학습 계획 (4주 집중 과정)

### 1주차: 기초 다지기
- **Day 1**: 타이머 기초 (`timer1~5.cpp`) - 동기/비동기 차이, 콜백 이해
- **Day 2**: 타이머 응용 (`timers` 폴더) - 타이머 기반 게임 틱 구현
- **Day 3**: TCP 동기 I/O (`blocking_tcp_echo_*.cpp`) - 클라이언트/서버 구조
- **Day 4**: TCP 비동기 기초 (`async_tcp_echo_server.cpp`) - async_accept/read/write 패턴
- **Day 5**: UDP 통신 (`async_udp_echo_server.cpp`) - 실시간 게임에서 UDP 활용

### 2주차: 핵심 패턴 마스터 (chat_server 집중)
- **Day 6-8**: `chat_server.cpp` 완전 분석 (3일 집중)
  - Day 6: 전체 구조 파악 (클래스 설계, 흐름)
  - Day 7: 세션 관리 (session 클래스, 생명주기)
  - Day 8: 메시지 브로드캐스트 (멀티 클라이언트 처리 핵심)
- **Day 9**: `chat_client.cpp` 분석 - 송수신 분리, 입력 처리
- **Day 10**: chat 서버 직접 구현 - 참고하여 스스로 구현

### 3주차: 고급 기법 & 프로젝트 설계
- **Day 11**: HTTP 서버 구조 (`http/server`) - 프로토콜 파싱, request/reply 구조
- **Day 12**: 메모리 최적화 (`allocation/server.cpp`) - handler allocator 패턴
- **Day 13**: Executor/스레드풀 (`executors`) - 멀티스레딩, 스레드 풀 관리
- **Day 14**: Spawn/코루틴 (`spawn`) - stackful coroutine으로 깔끔한 비동기 코드
- **Day 15**: 게임 서버 프로젝트 설계 - 프로토콜, 구조 설계

### 4주차: 실전 프로젝트 구현
- **Day 16**: 기본 서버 구현 - TCP 서버 + 세션 뼈대
- **Day 17**: 프로토콜 구현 - 패킷 헤더/바디 구조
- **Day 18**: 게임 로직 구현 - 로그인, 룸, 매칭
- **Day 19**: 테스트 & 디버깅 - 버그 수정, 개선
- **Day 20**: 복습 & 정리 - 핵심 패턴 정리, 문서화

## 코드 패턴

### 비동기 패턴 구조
```cpp
boost::asio::io_context io_context;
// async_accept -> async_read -> async_write 체인
// 핸들러에서 재귀적으로 다음 작업 시작
```

### 세션 관리
- `chat_server.cpp` 참조: `participant` 인터페이스 + `chat_session` 구현
- 멀티 클라이언트 관리를 위한 `chat_room` 패턴

### 스레드 안전성
- `strand` 사용하여 핸들러 직렬화
- `io_context::run()`을 여러 스레드에서 호출하여 스레드풀 구성

## 참고 예제 위치

모든 Boost.Asio 예제는 `C:\Work\Libs\boost_1_89_0\libs\asio\example\cpp11\` 하위에 위치합니다.