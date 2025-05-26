#pragma once



// _WIN32 , _WIN64 매크로는 MSVC 컴파일러가 Windows용으로 컴파일할 때 자동으로 정의됩니다.
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
#elif defined(__linux__)
    #define PLATFORM_LINUX
#else
    #error "지원하지 않는 플랫폼"
#endif

// -----------------------------------------------------
// 운영체제별 헤더 및 매크로 정의
// -----------------------------------------------------


#if defined(PLATFORM_WINDOWS)

    #define WIN32_LEAN_AND_MEAN // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.

    // Windows 전용 매크로: windows.h에 정의된 min/max 매크로와
    // C++ 표준 라이브러리의 std::min/std::max 함수의 이름 충돌을 방지합니다.
    #define NOMINMAX 

    // Windows API의 핵심 헤더 파일입니다.
    #include <windows.h>
    
    // Windows 소켓(Winsock) 관련 헤더 파일입니다.
    // winsock2.h는 ws2tcpip.h보다 먼저 포함되어야 합니다.
    #include <winsock2.h> 
    #include <ws2tcpip.h> // IPv6, getaddrinfo 등 최신 TCP/IP 함수를 위한 헤더
    #include <mswsock.h>  // Microsoft Winsock 확장을 위한 헤더 (AcceptEx, ConnectEx 등 사용 시 필요)


#elif defined(PLATFORM_LINUX) // _WIN32, _WIN64가 정의되지 않은 경우 (주로 Linux, macOS 등 Unix-like 시스템)
    
    #include <pthread.h>

    // Linux/Unix 소켓 프로그래밍에 필요한 표준 헤더 파일들입니다.
    #include <sys/socket.h>     // 소켓 관련 기본 함수 (socket, bind, listen, accept, connect 등)
    #include <netinet/in.h>     // 인터넷 주소 구조체 (sockaddr_in, sockaddr_in6) 및 상수 (IPPROTO_TCP 등)
    #include <netinet/tcp.h>    // TCP_NODELAY
    #include <arpa/inet.h>      // IP 주소 변환 함수 (inet_pton, inet_ntop 등)
    #include <unistd.h>         // POSIX 표준 함수 (close() 등, Windows의 closesocket()에 해당)
    #include <netdb.h>          // 호스트 이름/서비스 해석 함수 (getaddrinfo, freeaddrinfo 등)
    #include <fcntl.h>          // Non Blocking

    #include <sys/types.h>      // 다양한 데이터 타입 정의
    #include <sys/syscall.h>
    #include <cerrno>

    using SOCKET = int;
    constexpr int INVALID_SOCKET = -1;
    constexpr int SOCKET_ERROR = -1;

#endif

#include <map>
#include <set>
#include <list>
#include <array>
#include <queue>
#include <stack>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#include <mutex>
#include <thread>
#include <future>
#include <condition_variable>

#include <memory>
#include <string>
#include <iostream> 
#include <functional>

#include <cstring>
#include <cstdlib>
#include <cassert>

// 타입 정의 헤더 파일 
#include "Types.hpp"    
#include "Enums.hpp"
#include "ThreadLocal.hpp"

#include "Global.hpp"
#include "MemoryPool.hpp"
#include "ThreadManager.hpp"
#include "Lock.hpp"