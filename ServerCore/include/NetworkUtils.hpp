#pragma once

#include "NetworkAddress.hpp"

namespace servercore
{
	class NetworkUtils
	{
	public:
		static void Initialize();
		static void Clear();

	public:
		static SOCKET	CreateSocket(bool overlapped = true);
		static void		CloseSocket(SOCKET& socket);

		static bool		Bind(SOCKET socket, NetworkAddress networkAddress);
		static bool		Bind(SOCKET socket, uint16 port);
		static bool		Listen(SOCKET socket, int32 backlog = SOMAXCONN);

	public:
		static bool		SetLinger(SOCKET socket, uint16 onoff, uint16 linger);
		static bool		SetReuseAddress(SOCKET socket, bool flag);
		static bool		SetTcpNoDelay(SOCKET socket, bool flag);
		static bool		SetRecvBufferSize(SOCKET socket, int32 size);
		static bool		SetSendBufferSize(SOCKET socket, int32 size);

#if defined(PLATFORM_WINDOWS)
		static bool		SetUpdateAcceptSocket(SOCKET socket, SOCKET listenSocket);

	public:
		static LPFN_CONNECTEX		S_ConnectEx;
		static LPFN_DISCONNECTEX	S_DisconnectEx;
		static LPFN_ACCEPTEX		S_AcceptEx;
#endif
	};

	template<typename T>
	static inline bool SetSocketOpt(SOCKET socket, int32 level, int32 optName, T optVal)
	{
#if defined(PLATFORM_WINDOWS)
		return SOCKET_ERROR != ::setsockopt(socket, level, optName, reinterpret_cast<const char*>(&optVal), sizeof(T));
#else
		return SOCKET_ERROR != ::setsockopt(socket, level, optName, &optVal, sizeof(T));
#endif
	}
}