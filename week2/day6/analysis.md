# Week2 Day6: Chat Server 전체 구조 분석

## 개요
`chat_server.cpp`는 Boost.Asio의 고급 패턴들이 집약된 핵심 예제입니다. 실제 게임 서버 개발에 필요한 모든 패턴을 담고 있습니다.

## 핵심 클래스 구조 (4개 클래스)

### 1. chat_message (프로토콜)
```cpp
static constexpr std::size_t header_length = 4;     // 헤더 길이
static constexpr std::size_t max_body_length = 512; // 최대 메시지 크기
```
- **역할**: 네트워크 프로토콜 정의
- **패턴**: 헤더(4바이트) + 바디 구조
- **핵심**: `encode_header()`, `decode_header()` - 길이 정보를 헤더에 저장

### 2. chat_participant (인터페이스)
```cpp
class chat_participant {
public:
  virtual ~chat_participant() {}
  virtual void deliver(const chat_message& msg) = 0;
};
```
- **역할**: 메시지 수신자 추상화
- **패턴**: 순수 가상 함수 인터페이스
- **핵심**: 다형성을 통한 확장성 제공

### 3. chat_room (멀티캐스트 관리자)
```cpp
std::set<chat_participant_ptr> participants_;  // 참가자 목록
chat_message_queue recent_msgs_;               // 최근 메시지 100개
```
- **역할**: 멀티 클라이언트 관리 및 브로드캐스트
- **패턴**: Observer 패턴 변형
- **핵심**: `join()`, `leave()`, `deliver()` - 채팅방 생명주기 관리

### 4. chat_session (클라이언트 세션)
```cpp
class chat_session : public chat_participant,
                     public std::enable_shared_from_this<chat_session>
```
- **역할**: 개별 클라이언트 연결 관리
- **패턴**: RAII + shared_ptr 생명주기 관리
- **핵심**: 비동기 읽기/쓰기 체인

## 전체 데이터 흐름

```
클라이언트 연결 → chat_server::do_accept()
                ↓
         chat_session 생성 → room_.join()
                ↓
    do_read_header() → do_read_body() → room_.deliver()
                ↓                           ↓
         모든 participants에게 메시지 전파
                ↓
    각 session의 deliver() → do_write()
```

## 핵심 비동기 패턴

### 1. Accept 루프 (chat_server)
```cpp
void do_accept() {
    acceptor_.async_accept([this](error_code ec, tcp::socket socket) {
        if (!ec) {
            std::make_shared<chat_session>(std::move(socket), room_)->start();
        }
        do_accept();  // 재귀적 호출로 계속 연결 대기
    });
}
```

### 2. Read 체인 (chat_session)
```cpp
do_read_header() → do_read_body() → room_.deliver() → do_read_header()
```
- 헤더 → 바디 → 처리 → 다시 헤더 읽기
- 에러 발생 시 `room_.leave()` 호출하여 정리

### 3. Write 큐 (chat_session)
```cpp
bool write_in_progress = !write_msgs_.empty();
write_msgs_.push_back(msg);
if (!write_in_progress) {
    do_write();  // 큐가 비어있을 때만 쓰기 시작
}
```

## 메모리 관리 패턴

### shared_ptr + enable_shared_from_this
```cpp
auto self(shared_from_this());  // 람다에서 객체 생명주기 보장
```
- 비동기 작업 중 객체가 소멸되지 않도록 보장
- RAII 패턴으로 자동 리소스 정리

### 컨테이너 선택
- `std::set<chat_participant_ptr>`: 중복 방지, 빠른 삭제
- `std::deque<chat_message>`: 큐 구조, 양쪽 삽입/삭제 효율적

## 핵심 학습 포인트

1. **인터페이스 설계**: `chat_participant`로 확장성 확보
2. **생명주기 관리**: `shared_ptr` + `enable_shared_from_this` 패턴
3. **비동기 체인**: 헤더→바디→처리→반복 패턴
4. **에러 처리**: 모든 비동기 작업에서 일관된 정리 로직
5. **멀티캐스트**: 효율적인 브로드캐스트 구현

## 다음 단계 (Day 7)
세션 관리의 세부 구현과 생명주기를 심화 분석할 예정입니다.