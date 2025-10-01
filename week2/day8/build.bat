@echo off
echo ====================================
echo  Broadcast Chat Server 빌드
echo ====================================

REM 빌드 디렉토리 생성
if not exist build mkdir build
cd build

REM CMake 설정 및 빌드
echo CMake 설정 중...
cmake .. -G "Visual Studio 17 2022" -A x64

if %ERRORLEVEL% NEQ 0 (
    echo CMake 설정 실패! 다른 생성기로 시도합니다...
    cmake .. -G "MinGW Makefiles"
)

if %ERRORLEVEL% NEQ 0 (
    echo CMake 설정 실패!
    pause
    exit /b 1
)

echo 빌드 중...
cmake --build . --config Release

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ====================================
    echo  빌드 성공!
    echo ====================================
    echo 실행: build\Release\broadcast_chat_server.exe 9999 (MSVC)
    echo       build\broadcast_chat_server.exe 9999 (MinGW)
    echo 클라이언트: ..\day6\build\bin\Release\chat_client.exe localhost 9999
) else (
    echo 빌드 실패!
)

pause
