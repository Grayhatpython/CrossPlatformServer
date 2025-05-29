#include "Pch.hpp"
#include "NetworkUtils.hpp"

namespace servercore
{

#if defined(PLATFORM_WINDOWS)
	LPFN_CONNECTEX		NetworkUtils::S_ConnectEx;
	LPFN_DISCONNECTEX	NetworkUtils::S_DisconnectEx;
	LPFN_ACCEPTEX		NetworkUtils::S_AcceptEx;
#endif

	void NetworkUtils::Initialize()
	{
#if defined(PLATFORM_WINDOWS)
		WSADATA wsaData;
		assert(::WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);

		DWORD bytes = 0;
		SOCKET dummySocket = CreateSocket();

		GUID guid = WSAID_CONNECTEX;
		assert(::WSAIoctl(dummySocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), reinterpret_cast<LPVOID>(&S_ConnectEx), sizeof(S_ConnectEx), &bytes, NULL, NULL) != SOCKET_ERROR);

		guid = WSAID_DISCONNECTEX;
		assert(::WSAIoctl(dummySocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), reinterpret_cast<LPVOID>(&S_DisconnectEx), sizeof(S_DisconnectEx), &bytes, NULL, NULL) != SOCKET_ERROR);

		guid = WSAID_ACCEPTEX;
		assert(::WSAIoctl(dummySocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), reinterpret_cast<LPVOID>(&S_AcceptEx), sizeof(S_AcceptEx), &bytes, NULL, NULL) != SOCKET_ERROR);

		CloseSocket(dummySocket);
#endif
	}

	void NetworkUtils::Clear()
	{
#if defined(PLATFORM_WINDOWS)
		::WSACleanup();
#endif
	}

	SOCKET NetworkUtils::CreateSocket(bool overlapped)
	{
		SOCKET socket = INVALID_SOCKET;

#if defined(PLATFORM_WINDOWS)
		socket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, overlapped ? WSA_FLAG_OVERLAPPED : 0);
#else
		socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if (socket != INVALID_SOCKET && overlapped)
		{
			int32 flags = ::fcntl(socket, F_GETFL, 0);
			::fcntl(socket, F_SETFL, flags | O_NONBLOCK);
		}
#endif

		assert(socket != INVALID_SOCKET);
		return socket;
	}

	void NetworkUtils::CloseSocket(SOCKET& socket)
	{
		if (socket != INVALID_SOCKET)
		{
#if defined(PLATFORM_WINDOWS)
			::closesocket(socket);
#else
			::close(socket);
#endif
		}

		socket = INVALID_SOCKET;
	}

	bool NetworkUtils::Bind(SOCKET socket, NetworkAddress networkAddress)
	{
		return SOCKET_ERROR != ::bind(socket, reinterpret_cast<struct sockaddr*>(&networkAddress.GetSocketAddress()), sizeof(struct sockaddr_in));
	}

	bool NetworkUtils::Bind(SOCKET socket, uint16 port)
	{
		struct sockaddr_in address;
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = ::htonl(INADDR_ANY);
		address.sin_port = ::htons(port);

		return SOCKET_ERROR != ::bind(socket, reinterpret_cast<const struct sockaddr*>(&address), sizeof(address));
	}

	bool NetworkUtils::Listen(SOCKET socket, int32 backlog)
	{
		return SOCKET_ERROR != ::listen(socket, backlog);
	}

	bool NetworkUtils::SetLinger(SOCKET socket, uint16 onoff, uint16 linger)
	{
#if defined(PLATFORM_WINDOWS)
		LINGER opt;
#else
		struct linger opt;
#endif
		opt.l_onoff = onoff;
		opt.l_linger = linger;
		return SetSocketOpt(socket, SOL_SOCKET, SO_LINGER, opt);
	}

	bool NetworkUtils::SetReuseAddress(SOCKET socket, bool flag)
	{
		int optVal = flag ? 1 : 0;
		return SetSocketOpt(socket, SOL_SOCKET, SO_REUSEADDR, optVal);
	}

	bool NetworkUtils::SetTcpNoDelay(SOCKET socket, bool flag)
	{
		int optVal = flag ? 1 : 0;
		return SetSocketOpt(socket, IPPROTO_TCP, TCP_NODELAY, optVal);
	}

	bool NetworkUtils::SetRecvBufferSize(SOCKET socket, int32 size)
	{
		return SetSocketOpt(socket, SOL_SOCKET, SO_RCVBUF, size);
	}

	bool NetworkUtils::SetSendBufferSize(SOCKET socket, int32 size)
	{
		return SetSocketOpt(socket, SOL_SOCKET, SO_SNDBUF, size);
	}

#if defined(PLATFORM_WINDOWS)
	bool NetworkUtils::SetUpdateAcceptSocket(SOCKET socket, SOCKET listenSocket)
	{
		return SetSocketOpt(socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, listenSocket);
	}
#endif

}