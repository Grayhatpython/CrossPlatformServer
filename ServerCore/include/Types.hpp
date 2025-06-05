#pragma once

#include <cstdint> 

using BYTE = unsigned char; 

#if defined(_WIN32) || defined(_WIN64)
	using int8 = __int8;
	using int16 = __int16;
	using int32 = __int32;
	using int64 = __int64;
	using uint8 = unsigned __int8;
	using uint16 = unsigned __int16;
	using uint32 = unsigned __int32;
	using uint64 = unsigned __int64;

	constexpr uint32 TIMEOUT_INFINITE = INFINITE;

#elif defined(PLATFORM_LINUX)
	using int8 = int8_t;
	using int16 = int16_t;
	using int32 = int32_t;
	using int64 = int64_t;
	using uint8 = uint8_t;
	using uint16 = uint16_t;
	using uint32 = uint32_t;
	using uint64 = uint64_t;
	using SOCKET = int;
	using HANDLE = int;
	using FileDescriptor = int;

	constexpr uint32 TIMEOUT_INFINITE = -1;
	constexpr SOCKET INVALID_SOCKET = -1;
	constexpr SOCKET SOCKET_ERROR = -1;
	constexpr HANDLE INVALID_HANDLE_VALUE = -1;
	constexpr FileDescriptor INVALID_FILE_DESCRIPTOR_VALUE = -1;

#endif