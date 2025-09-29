#!/usr/bin/env python3
import socket
import time
import threading

def udp_test_client():
    """UDP 에코 서버 테스트 클라이언트"""

    # UDP 소켓 생성
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server_address = ('localhost', 8080)

    print("UDP 테스트 클라이언트 시작")
    print("서버 주소:", server_address)
    print("'quit'을 입력하면 종료됩니다.")
    print("-" * 40)

    try:
        while True:
            message = input("메시지 입력: ")

            if message.lower() == 'quit':
                break

            if message:
                # 메시지 전송
                print(f"전송: {message}")
                sock.sendto(message.encode('utf-8'), server_address)

                # 응답 수신 (타임아웃 설정)
                sock.settimeout(5.0)
                try:
                    data, server = sock.recvfrom(1024)
                    print(f"응답: {data.decode('utf-8')}")
                except socket.timeout:
                    print("응답 시간 초과!")
                except Exception as e:
                    print(f"수신 오류: {e}")

                print("-" * 40)

    except KeyboardInterrupt:
        print("\n클라이언트 종료")
    finally:
        sock.close()

def auto_test():
    """자동 테스트 함수"""
    print("=== 자동 테스트 시작 ===")

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server_address = ('localhost', 8080)

    test_messages = [
        "Hello UDP Server!",
        "Test message 1",
        "한글 메시지 테스트",
        "Long message: " + "A" * 100,
        "Special chars: !@#$%^&*()"
    ]

    try:
        for i, message in enumerate(test_messages, 1):
            print(f"테스트 {i}: {message[:50]}{'...' if len(message) > 50 else ''}")

            # 메시지 전송
            sock.sendto(message.encode('utf-8'), server_address)

            # 응답 수신
            sock.settimeout(3.0)
            try:
                data, server = sock.recvfrom(1024)
                response = data.decode('utf-8')

                if response == message:
                    print("✅ 성공 - 에코 응답 일치")
                else:
                    print("❌ 실패 - 에코 응답 불일치")
                    print(f"  예상: {message}")
                    print(f"  실제: {response}")
            except socket.timeout:
                print("❌ 실패 - 응답 시간 초과")
            except Exception as e:
                print(f"❌ 실패 - 오류: {e}")

            time.sleep(0.5)  # 잠시 대기

    except Exception as e:
        print(f"자동 테스트 오류: {e}")
    finally:
        sock.close()

    print("=== 자동 테스트 완료 ===")

if __name__ == "__main__":
    print("1. 대화형 테스트")
    print("2. 자동 테스트")
    choice = input("선택 (1 또는 2): ")

    if choice == "2":
        auto_test()
    else:
        udp_test_client()