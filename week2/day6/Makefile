# Chat Server Makefile
# Week2 Day6 - Boost.Asio Chat Server

CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2
BOOST_INCLUDE = C:\Work\Libs\boost_1_89_0
INCLUDES = -I$(BOOST_INCLUDE)

# Windows용 설정
ifeq ($(OS),Windows_NT)
	LDFLAGS = -lws2_32
	EXE_EXT = .exe
else
	LDFLAGS = -lpthread
	EXE_EXT =
endif

TARGET = chat_server$(EXE_EXT)
SOURCES = chat_server.cpp
HEADERS = chat_message.hpp

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) -o $(TARGET) $(LDFLAGS)

# MSVC 컴파일 (Windows)
msvc:
	cl /EHsc /std:c++17 /I "$(BOOST_INCLUDE)" chat_server.cpp /Fe:chat_server_msvc.exe

# 테스트 실행 (포트 9999)
test: $(TARGET)
	./$(TARGET) 9999

# 정리
clean:
	rm -f $(TARGET) chat_server_msvc.exe *.obj

# 도움말
help:
	@echo "사용 가능한 명령:"
	@echo "  make        - g++로 컴파일"
	@echo "  make msvc   - MSVC로 컴파일"
	@echo "  make test   - 컴파일 후 포트 9999에서 테스트 실행"
	@echo "  make clean  - 생성된 파일 정리"

.PHONY: all msvc test clean help