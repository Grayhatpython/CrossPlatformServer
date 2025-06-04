#pragma once

// _WIN32 , _WIN64 매크로는 MSVC 컴파일러가 Windows용으로 컴파일할 때 자동으로 정의됩니다.
#if defined(_WIN32) || defined(_WIN64)
#define PLATFORM_WINDOWS
#elif defined(__linux__)
#define PLATFORM_LINUX
#else
#error "Platform not supported"
#endif
