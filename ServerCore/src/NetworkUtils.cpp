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
#else
		//	클라이언트가 비정상적인 종료나, 네트워크 연결이 갑자기 끊어졌는 상황에 서버는 아직 그 상황을 감지못하고 해당 클라이언트에게 데이터를 보내기 위해
		//	send()또는 write() 호출 시 리눅스 커널을 상대방이 없는 상황인데 데이터를 쓰려고 하네? 라는 비정상적인 상황에 대해 해당 서버 프로세스에게 SIGPIPE 신호를 보낸다.
		//	SIGPIPE 신호 -> 서버 프로세스 강제 종료 -> 나머지 정상적인 연결의 클라이언트들을 서비스하던 서버 전체가 다운?
		//	그래서 커널에게 SIGPIPE 신호가 오면, 그냥 무시해라~
		//	위 상황에서 서버가 send() 호출 시 커널이 SIGPIPE 신호를 보내면 무시하고 대신 send() or write() 함수가 즉시 실패하고 -1을 반환한다
		//	이때 전역 변수 errno에는 EPIPE 라는 에러 코드가 설정된다.
		//	이제 서버는 프로세스가 죽는 대신, errno 값을 통해, 이 클라이언트와의 연결이 끊어졌구나 라는 사실을 감지하고 disconnect 과정을 하면된다.
		::signal(SIGPIPE, SIG_IGN);
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

		if(overlapped == true)
		{
#if defined(PLATFORM_WINDOWS)
			socket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, overlapped ? WSA_FLAG_OVERLAPPED : 0);
#else
		socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		assert(SetNonBlocking(socket));
#endif
		}
		else
		{
			socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		}

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

    bool NetworkUtils::SetNonBlocking(SOCKET socket)
    {
#if defined(PLATFORM_WINDOWS)
		u_long mode = 1;
		if(::ioctlsocket(socket, FIONBIO, &mode) != NO_ERROR)
			return false;
#else
		if (socket != INVALID_SOCKET)
		{
			int32 flags = ::fcntl(socket, F_GETFL, 0);
			if(flags < 0)
            	return false; 
				
			if(::fcntl(socket, F_SETFL, flags | O_NONBLOCK) < 0)
            	return false;
		}
#endif
		return true;
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