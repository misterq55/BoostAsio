# Week2 Day8: 메시지 브로드캐스트 (멀티 클라이언트 처리 핵심)

## 개요
멀티 클라이언트 환경에서 메시지를 모든 참가자에게 전송하는 브로드캐스트 메커니즘을 학습합니다.

## 학습 내용
- `chat_room::deliver()` 브로드캐스트 구조
- 각 세션의 독립적인 쓰기 큐 관리
- 큐 포화 문제와 백프레셔 대응
- 개선된 서버 구현 (큐 제한, 모니터링)

## 빌드 및 실행

### 자동 빌드
```bash
build.bat
```

### 수동 빌드
```bash
cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
```

### 실행
```bash
# 서버
build\broadcast_chat_server.exe 9999

# 클라이언트 (여러 개 실행)
..\day6\build\chat_client.exe localhost 9999
```

## 핵심 개선 사항
1. **큐 크기 제한**: 100개 초과시 연결 종료
2. **큐 상태 모니터링**: 10개 이상 누적시 경고
3. **통계 수집**: 브로드캐스트 횟수, 참가자 수 출력

## 주요 파일
- `broadcast_chat_server.cpp`: 개선된 브로드캐스트 서버
- `broadcast_analysis.md`: 상세 분석 문서
- `CMakeLists.txt`: 빌드 설정
- `build.bat`: 자동 빌드 스크립트

## 학습 순서
1. `broadcast_analysis.md` 읽기
2. 서버/클라이언트 실행하여 동작 확인
3. 코드의 개선 부분 (★ 표시) 찾아보기
4. 3개 이상 클라이언트로 브로드캐스트 테스트

## 다음 단계
- Day 9: `chat_client.cpp` 분석
