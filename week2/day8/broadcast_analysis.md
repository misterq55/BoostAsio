# Week2 Day8: 메시지 브로드캐스트 심화 분석

## 학습 목표
- 멀티 클라이언트 환경에서 메시지 브로드캐스트 메커니즘 이해
- 각 세션의 독립적인 쓰기 큐 관리 방식 마스터
- 큐 포화 문제와 백프레셔(Backpressure) 대응 전략 학습

---

## 1. 브로드캐스트 핵심 흐름

### 1.1 전체 흐름도
```
클라이언트A 메시지 전송
    ↓
chat_session::do_read_body() (128번 라인)
    ↓
room_.deliver(read_msg_)  ← 브로드캐스트 트리거
    ↓
chat_room::deliver() (55-63번 라인)
    ├─→ participants_[A]->deliver(msg)  → 자신에게도 전송
    ├─→ participants_[B]->deliver(msg)
    └─→ participants_[C]->deliver(msg)
          ↓
    각 세션의 write_msgs_ 큐에 추가
          ↓
    do_write() → async_write 체인
```

### 1.2 코드 위치 정리
| 단계 | 함수 | 라인 | 역할 |
|------|------|------|------|
| 1 | `chat_session::do_read_body()` | 119-136 | 메시지 수신 완료 후 room에 전달 |
| 2 | `chat_room::deliver()` | 55-63 | 모든 참가자에게 브로드캐스트 |
| 3 | `chat_session::deliver()` | 90-98 | 개별 세션 큐에 추가 |
| 4 | `chat_session::do_write()` | 138-159 | 비동기 전송 |

---

## 2. chat_room::deliver() 상세 분석

### 2.1 히스토리 관리
```cpp
void deliver(const chat_message& msg)
{
    // 1. 히스토리 저장 (FIFO)
    recent_msgs_.push_back(msg);
    while (recent_msgs_.size() > max_recent_msgs)
        recent_msgs_.pop_front();  // 100개 초과시 오래된 메시지 제거

    // 2. 브로드캐스트
    for (auto participant: participants_)
        participant->deliver(msg);
}
```

**Why 히스토리?**
- 신규 참가자가 `join()`할 때 최근 100개 메시지 전송 (46-47번)
- 채팅방 컨텍스트 제공
- `std::deque` 사용으로 효율적인 FIFO 구현

### 2.2 브로드캐스트 루프
```cpp
for (auto participant: participants_)  // std::set 순회
    participant->deliver(msg);         // 가상 함수 호출
```

**설계 포인트:**
1. **인터페이스 기반 설계**: `chat_participant` 추상 클래스
2. **다형성**: 세션뿐 아니라 봇, 로거 등 추가 가능
3. **std::set 사용**: 중복 방지, 자동 정렬 (shared_ptr 주소 기준)

---

## 3. 멀티 클라이언트 메시지 큐 관리 (핵심!)

### 3.1 각 세션의 독립적인 큐

```cpp
class chat_session {
    tcp::socket socket_;
    chat_message read_msg_;        // 읽기 버퍼 (1개만 필요)
    chat_message_queue write_msgs_; // 쓰기 큐 (여러 개 대기 가능)
};
```

**Why 독립적인 큐?**
- 클라이언트마다 네트워크 속도가 다름
- 느린 클라이언트가 전체 브로드캐스트를 막으면 안 됨
- 각자의 큐에서 자기 속도에 맞춰 소비

### 3.2 큐 관리 메커니즘 (핵심 로직!)

```cpp
void deliver(const chat_message& msg) {
    bool write_in_progress = !write_msgs_.empty();  // ① 쓰기 중?
    write_msgs_.push_back(msg);                      // ② 큐에 추가
    if (!write_in_progress)                          // ③ 유휴 상태면
        do_write();                                  //    즉시 전송 시작
}
```

**상태 다이어그램:**
```
[유휴 상태] write_msgs_.empty() == true
    ↓ deliver() 호출
    ↓ do_write() 즉시 시작
[쓰기 중] write_msgs_.size() > 0
    ↓ deliver() 호출 (추가 메시지)
    ↓ 큐에만 추가, do_write() 호출 안 함
    ↓ 현재 async_write 완료 후 자동으로 다음 메시지 전송
[유휴 상태] 복귀
```

**Why `write_in_progress` 플래그?**
- `async_write`는 동시에 하나만 실행 가능
- 진행 중일 때 새 메시지는 큐에만 추가
- 완료 핸들러에서 `pop_front()` 후 재귀 호출

### 3.3 do_write() 체인

```cpp
void do_write() {
    async_write(socket_, buffer(write_msgs_.front()),
        [this, self](error_code ec, size_t) {
            if (!ec) {
                write_msgs_.pop_front();     // ① 완료된 메시지 제거
                if (!write_msgs_.empty())    // ② 큐에 남았으면
                    do_write();              // ③ 재귀 호출
            } else {
                room_.leave(shared_from_this());
            }
        });
}
```

**재귀 구조의 장점:**
- 명시적인 루프 없이 큐 처리
- 비동기 완료 시점에 자동으로 다음 작업
- 코드 간결성

---

## 4. 문제점 및 개선 방향

### 4.1 원본 코드의 문제점

| 문제 | 원인 | 결과 |
|------|------|------|
| **무한 큐 증가** | 큐 크기 제한 없음 | 메모리 누수, 느린 클라이언트가 서버 다운 |
| **백프레셔 미흡** | 큐 상태 모니터링 없음 | 문제 발생 시점 파악 어려움 |
| **통계 부족** | 브로드캐스트 횟수 미집계 | 성능 분석 곤란 |

### 4.2 개선된 broadcast_chat_server.cpp

#### 1) 큐 크기 제한
```cpp
void deliver(const chat_message& msg) override {
    if (write_msgs_.size() >= max_outbound_msgs_) {  // 100개 제한
        socket_.close();  // 연결 강제 종료
        return;
    }
    // ... 기존 로직
}
```

#### 2) 큐 상태 모니터링
```cpp
if (write_msgs_.size() > 10) {
    std::cout << "[WARNING] 쓰기 큐 누적: " << write_msgs_.size() << "\n";
}
```

#### 3) 통계 수집
```cpp
class chat_room {
    size_t broadcast_count_ = 0;

    void deliver(const chat_message& msg) {
        broadcast_count_++;
        // ... 브로드캐스트
    }
};
```

---

## 5. 핵심 패턴 정리

### 5.1 생산자-소비자 패턴
- **생산자**: `chat_room::deliver()` - 모든 세션에 메시지 투입
- **소비자**: `chat_session::do_write()` - 각자 속도로 전송
- **버퍼**: `write_msgs_` 큐

### 5.2 비동기 체인 패턴
```cpp
do_read_header() → do_read_body() → room_.deliver() → do_write() → do_write() → ...
```

### 5.3 백프레셔 패턴
```
메시지 생산 속도 > 소비 속도
    ↓
큐 증가
    ↓
임계값 초과
    ↓
연결 종료 또는 속도 제한
```

---

## 6. 실습 가이드

### 6.1 빌드 및 실행
```bash
cd week2/day8
build.bat

# 서버 실행
build\broadcast_chat_server.exe 9999

# 클라이언트 여러 개 실행 (다른 터미널에서)
..\day6\build\chat_client.exe localhost 9999
```

### 6.2 테스트 시나리오

#### 시나리오 1: 정상 브로드캐스트
1. 클라이언트 3개 접속
2. A가 메시지 전송
3. B, C에서 즉시 수신 확인
4. 서버 로그: `[BROADCAST] 3명에게 메시지 전송`

#### 시나리오 2: 큐 포화 테스트
1. 네트워크 지연 시뮬레이션 (클라이언트 sleep 추가)
2. 빠르게 100개 이상 메시지 전송
3. 큐 경고 및 연결 종료 확인

#### 시나리오 3: 히스토리 확인
1. 클라이언트 A, B가 대화 (100개 메시지)
2. 클라이언트 C 새로 접속
3. C가 최근 100개 메시지 즉시 수신 확인

---

## 7. 게임 서버 적용 포인트

### 7.1 채널/방 시스템
```cpp
class game_room {
    std::set<player_ptr> players_;

    void broadcast_state() {
        for (auto player : players_)
            player->send_state(game_state_);
    }
};
```

### 7.2 관심 영역 브로드캐스트 (AOI)
```cpp
void broadcast_position(player_ptr source) {
    for (auto target : nearby_players(source->position())) {
        target->send_position_update(source->position());
    }
}
```

### 7.3 우선순위 큐
```cpp
struct message {
    enum priority { LOW, NORMAL, HIGH };
    priority prio;
};

std::priority_queue<message> write_msgs_;  // 중요 메시지 먼저
```

---

## 8. 추가 학습 과제

1. **멀티스레드 브로드캐스트**: strand 사용하여 동시성 제어
2. **선택적 브로드캐스트**: 특정 조건(팀, 채널)만 전송
3. **메시지 압축**: 대역폭 절약 (zlib, lz4)
4. **델타 압축**: 이전 상태와 차이만 전송 (게임 상태 동기화)

---

## 9. 핵심 요약

| 개념 | 핵심 내용 |
|------|-----------|
| **브로드캐스트** | `for (auto p : participants_) p->deliver(msg);` |
| **독립 큐** | 각 세션마다 `write_msgs_` 큐 보유 |
| **쓰기 체인** | `do_write()` 완료 → `pop_front()` → 재귀 |
| **큐 제한** | `max_outbound_msgs` 초과시 연결 종료 |
| **히스토리** | 신규 참가자에게 최근 100개 전송 |

**오늘의 핵심 한 줄:**
> 브로드캐스트는 간단하지만, 각 클라이언트의 독립적인 쓰기 큐 관리가 멀티 클라이언트 서버의 핵심이다!

---

## 다음 단계
- **Day 9**: `chat_client.cpp` 분석 - 송수신 분리, 사용자 입력 처리
- **Day 10**: chat 서버 직접 구현 - 참고하여 스스로 구현
