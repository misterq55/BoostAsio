# Week2 Day7: 세션 관리 심화 분석

## 개요
`chat_server.cpp`의 `chat_session` 클래스를 중심으로 세션 생명주기, 메모리 관리, 비동기 작업 중 객체 안전성을 분석합니다.

---

## 1. chat_session 생명주기

### 1.1 세션 생성
```cpp
// chat_server.cpp:187
std::make_shared<chat_session>(std::move(socket), room_)->start();
```

**생성 과정**:
1. 클라이언트가 서버에 접속하면 `do_accept()` 콜백 호출
2. `std::make_shared`로 `chat_session` 객체 생성 (ref_count = 1)
3. 즉시 `start()` 메서드 호출
4. 임시 `shared_ptr` 소멸 → **하지만 세션은 살아있음!** (아래 설명)

### 1.2 세션 초기화
```cpp
// chat_server.cpp:84-88
void start()
{
    room_.join(shared_from_this());  // chat_room에 등록
    do_read_header();                // 첫 읽기 시작
}
```

**핵심 메커니즘**:
- `room_.join()`에서 `chat_room`의 `participants_` set에 `shared_ptr` 저장
- 이로 인해 ref_count가 증가하여 임시 변수가 소멸해도 객체 유지
- `do_read_header()`로 비동기 읽기 체인 시작

### 1.3 세션 활동 기간
```cpp
// chat_server.cpp:101-117
void do_read_header()
{
    auto self(shared_from_this());  // 생명 연장
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.data(), chat_message::header_length),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
            if (!ec && read_msg_.decode_header())
            {
                do_read_body();
            }
            else
            {
                room_.leave(shared_from_this());  // 에러 시 종료
            }
        });
}
```

**동작 흐름**:
- 헤더 읽기 → 바디 읽기 → 메시지 브로드캐스트 → 다시 헤더 읽기 (무한 반복)
- 에러 발생 시 `room_.leave()` 호출하여 종료 시작

### 1.4 세션 종료
```cpp
// chat_server.cpp:50-53
void leave(chat_participant_ptr participant)
{
    participants_.erase(participant);  // set에서 제거
}
```

**종료 과정**:
1. `room_.leave()` 호출 → `participants_` set에서 제거
2. ref_count 감소 (set이 보유하던 참조 해제)
3. 모든 비동기 작업 완료 후 ref_count가 0이 되면 자동 소멸
4. 소켓도 자동으로 닫힘 (RAII 패턴)

---

## 2. shared_from_this 패턴

### 2.1 왜 필요한가?

```cpp
// chat_server.cpp:73-75
class chat_session
  : public chat_participant,
    public std::enable_shared_from_this<chat_session>
```

**문제 상황**:
- 비동기 작업이 진행 중일 때 객체가 삭제되면 댕글링 포인터 발생
- `this`만 캡처하면 raw pointer이므로 생명주기 보장 안 됨

**해결책**:
- `enable_shared_from_this`를 상속하여 `shared_from_this()` 사용 가능
- 자기 자신을 가리키는 `shared_ptr`를 안전하게 생성

### 2.2 사용 패턴

코드에서 `shared_from_this()`가 사용되는 5곳:

#### 1) start() - 방 등록
```cpp
// chat_server.cpp:86
room_.join(shared_from_this());
```
- `chat_room`에 자신을 등록하여 메시지 수신 가능하게 함
- ref_count 증가 → 세션 생명 보장

#### 2) do_read_header() - 비동기 읽기
```cpp
// chat_server.cpp:103
auto self(shared_from_this());
boost::asio::async_read(socket_, ...,
    [this, self](boost::system::error_code ec, std::size_t /*length*/)
    {
        // this: 멤버 접근용 (raw pointer)
        // self: 생명 연장용 (shared_ptr)
    });
```

#### 3) do_read_body() - 비동기 읽기
```cpp
// chat_server.cpp:121
auto self(shared_from_this());
boost::asio::async_read(socket_, ..., [this, self](...) { ... });
```

#### 4) do_write() - 비동기 쓰기
```cpp
// chat_server.cpp:140
auto self(shared_from_this());
boost::asio::async_write(socket_, ..., [this, self](...) { ... });
```

#### 5) 에러 처리 - 방 퇴장
```cpp
// chat_server.cpp:114, 133, 156
room_.leave(shared_from_this());
```

### 2.3 this vs self

```cpp
[this, self](boost::system::error_code ec, std::size_t length)
{
    if (!ec)
    {
        room_.deliver(read_msg_);  // this를 통한 멤버 접근
        do_read_header();          // this를 통한 메서드 호출
    }
    // self는 명시적으로 사용하지 않지만,
    // 람다가 살아있는 동안 객체를 살려두는 역할!
}
```

**역할 분리**:
- `this`: 멤버 변수/메서드 접근용 (raw pointer)
- `self`: 객체 생명 연장용 (shared_ptr, 명시적 사용 없음)

**핵심 원리**:
- 람다가 `self`를 캡처하면 `shared_ptr`의 복사본을 가짐
- 비동기 작업이 완료될 때까지 람다 객체가 살아있음
- 따라서 `self`의 ref_count가 유지되어 세션 객체가 살아있음

---

## 3. 메모리 관리 메커니즘

### 3.1 참조 카운트 추적

#### 시나리오: 클라이언트 접속 → 메시지 전송 → 종료

```cpp
// 1. 생성 (ref_count = 1)
std::make_shared<chat_session>(std::move(socket), room_)->start();

// 2. room_.join() 호출 (ref_count = 2)
room_.join(shared_from_this());  // participants_ set에 추가

// 3. do_read_header() 호출 (ref_count = 3)
auto self(shared_from_this());
async_read(..., [this, self](...) { ... });

// 4. 임시 변수 소멸 (ref_count = 2)
// main의 make_shared 임시 객체 소멸

// 5. 비동기 읽기 완료 (ref_count = 2)
// 람다 소멸하면서 self 해제 → ref_count = 2

// 6. 클라이언트 연결 끊김 (ref_count = 1)
room_.leave(shared_from_this());  // participants_ set에서 제거

// 7. 마지막 비동기 작업 완료 (ref_count = 0)
// 람다 소멸 → 객체 자동 삭제
```

### 3.2 생명주기 다이어그램

```
[클라이언트 접속]
    ↓
[make_shared] ref_count = 1
    ↓
[start()] → [room_.join()] ref_count = 2
    ↓
[do_read_header()] ref_count = 3 (람다 캡처)
    ↓
[임시 변수 소멸] ref_count = 2
    ↓
[비동기 읽기 완료] ref_count = 2 (람다 소멸)
    ↓
[메시지 수신/전송 반복...]
    ↓
[에러 or 연결 끊김]
    ↓
[room_.leave()] ref_count = 1 (set에서 제거)
    ↓
[마지막 비동기 작업 완료] ref_count = 0
    ↓
[소멸자 호출, 소켓 자동 닫힘]
```

### 3.3 메모리 누수 방지

#### 1) 순환 참조 없음
```cpp
class chat_session {
    chat_room& room_;  // 참조로 보유 (shared_ptr 아님!)
};
```
- `chat_room`은 `chat_session`을 `shared_ptr`로 보유
- `chat_session`은 `chat_room`을 참조로 보유
- 단방향 소유 관계 → 순환 참조 없음

#### 2) 비동기 작업 중 안전한 종료
```cpp
auto self(shared_from_this());
async_read(..., [this, self](...) {
    // 작업 중 다른 곳에서 room_.leave() 호출해도
    // self가 살려두므로 안전하게 완료 가능
});
```

#### 3) RAII 패턴
```cpp
tcp::socket socket_;  // 객체 소멸 시 자동으로 소켓 닫힘
```

---

## 4. 세션 종료 시나리오

### 4.1 정상 종료 (클라이언트가 연결 끊음)

```cpp
// 1. 클라이언트가 소켓 닫음
client_socket.close();

// 2. 서버의 async_read가 에러 반환 (EOF)
[this, self](boost::system::error_code ec, std::size_t length)
{
    if (!ec && read_msg_.decode_header())  // ec가 EOF
    {
        // 실행 안 됨
    }
    else
    {
        room_.leave(shared_from_this());  // 방에서 퇴장
    }
}

// 3. participants_ set에서 제거 → ref_count 감소
void leave(chat_participant_ptr participant)
{
    participants_.erase(participant);
}

// 4. 모든 비동기 작업 완료 후 자동 소멸
```

### 4.2 비정상 종료 (네트워크 에러)

```cpp
// 1. 네트워크 에러 발생 (타임아웃, 연결 끊김 등)
// 2. async_read/async_write가 에러 코드 반환
// 3. 모든 비동기 핸들러에서 room_.leave() 호출
if (!ec)
{
    // 정상 처리
}
else
{
    room_.leave(shared_from_this());  // 에러 시 즉시 퇴장
}
```

**에러 처리 위치** (3곳):
- `do_read_header()` - chat_server.cpp:114
- `do_read_body()` - chat_server.cpp:133
- `do_write()` - chat_server.cpp:156

### 4.3 종료 타이밍 보장

**중요**: `room_.leave()` 호출 후에도 객체가 즉시 소멸하지 않음!

```cpp
// 상황: async_write 진행 중 async_read에서 에러 발생
do_read_header():
    [this, self](...) {
        room_.leave(shared_from_this());  // ref_count = 2 → 1
        // 하지만 do_write의 람다가 아직 self를 보유 중!
    }

do_write():
    [this, self](...) {
        // 이 람다가 완료될 때까지 객체 유지
        write_msgs_.pop_front();
        // 완료 후 self 소멸 → ref_count = 0 → 객체 삭제
    }
```

**장점**:
- 진행 중인 비동기 작업들이 안전하게 완료됨
- 명시적인 취소/대기 코드 불필요

---

## 5. 핵심 패턴 정리

### 5.1 비동기 작업 생명주기 패턴

```cpp
void some_async_operation()
{
    auto self(shared_from_this());  // 1. 자신의 shared_ptr 생성

    boost::asio::async_xxx(          // 2. 비동기 작업 시작
        resource_,
        [this, self](error_code ec)  // 3. 람다에 this와 self 캡처
        {
            if (!ec)
            {
                // this로 멤버 접근
                some_async_operation();  // 4. 다음 작업 체인
            }
            else
            {
                cleanup();  // 5. 에러 시 정리
            }
            // 6. 람다 소멸 시 self도 소멸 (ref_count 감소)
        });
}
```

### 5.2 세션 관리 체크리스트

#### ✅ 올바른 패턴
- `enable_shared_from_this` 상속
- 비동기 핸들러에서 `auto self(shared_from_this())` 캡처
- `chat_room`에 `shared_ptr`로 등록하여 생명 보장
- 에러 시 `room_.leave()` 호출하여 정리

#### ❌ 피해야 할 실수
- `this`만 캡처 (생명주기 보장 안 됨)
- `shared_from_this()` 없이 `make_shared`만 사용
- 순환 참조 생성 (양방향 `shared_ptr` 보유)
- `room_.leave()` 누락 (메모리 누수)

### 5.3 응용: 타임아웃 추가

```cpp
class chat_session : ... {
private:
    boost::asio::steady_timer timeout_timer_;

    void start_timeout()
    {
        auto self(shared_from_this());
        timeout_timer_.expires_after(std::chrono::seconds(30));
        timeout_timer_.async_wait(
            [this, self](boost::system::error_code ec)
            {
                if (!ec)  // 타임아웃 발생
                {
                    room_.leave(shared_from_this());
                }
            });
    }

    void cancel_timeout()
    {
        timeout_timer_.cancel();
    }
};
```

---

## 6. 실습 문제

### 문제 1: 로그 추가
세션 생성/소멸 시 로그를 출력하도록 수정하세요.
```cpp
chat_session(tcp::socket socket, chat_room& room)
    : socket_(std::move(socket)), room_(room)
{
    std::cout << "Session created: " << socket_.remote_endpoint() << "\n";
}

~chat_session()
{
    std::cout << "Session destroyed\n";
}
```

### 문제 2: 참조 카운트 출력
각 비동기 작업 시작 시 현재 ref_count를 출력하세요.
```cpp
void do_read_header()
{
    auto self(shared_from_this());
    std::cout << "ref_count: " << self.use_count() << "\n";
    // ...
}
```

### 문제 3: 세션 통계
`chat_room`에 현재 접속자 수를 반환하는 메서드를 추가하세요.
```cpp
class chat_room {
public:
    size_t participant_count() const
    {
        return participants_.size();
    }
};
```

---

## 7. 다음 단계

**Day 8 (내일)**: 메시지 브로드캐스트 심화
- `deliver()` 메서드 분석
- 쓰기 큐 관리 (`write_msgs_`)
- 멀티 클라이언트 동시 전송 처리
- 쓰기 경합 조건 방지 메커니즘

**핵심 질문**:
- 왜 `deliver()`에서 `write_in_progress` 플래그가 필요한가?
- 여러 클라이언트가 동시에 메시지를 보내면 어떻게 처리되는가?
- 쓰기 큐가 무한정 커지는 것을 어떻게 방지할 수 있는가?