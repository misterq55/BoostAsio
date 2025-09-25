#!/usr/bin/env python3
import socket
import time

def test_async_server():
    try:
        # 서버에 연결
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(('localhost', 12345))

        print("서버에 연결되었습니다!")

        # 테스트 메시지 전송
        test_messages = [
            "Hello Async Server!",
            "비동기 패턴 테스트",
            "Day 4 학습 완료"
        ]

        for msg in test_messages:
            print(f"전송: {msg}")
            sock.send(msg.encode('utf-8'))

            # 응답 받기
            response = sock.recv(1024).decode('utf-8')
            print(f"응답: {response}")

            time.sleep(0.5)

        sock.close()
        print("테스트 완료!")

    except Exception as e:
        print(f"오류 발생: {e}")

if __name__ == "__main__":
    test_async_server()