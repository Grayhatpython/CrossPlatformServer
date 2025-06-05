#include "Pch.hpp"
#include "Session.hpp"
#include "ServerCore.hpp"
#include "Packet.hpp"

namespace servercore
{
#if defined(PLATFORM_WINDOWS)
	std::atomic<uint64> Session::S_GenerateSessionId = 1;

	Session::Session()
	{
		_sessionId = S_GenerateSessionId.fetch_add(1);
	}

	Session::~Session()
	{
		//	TEST
		std::cout << "~Session" << std::endl;

		CloseSocket();
		
		//	TEMP
		while (_sendContextQueue.empty() == false)
			_sendContextQueue.pop();
	}

	bool Session::Connect(NetworkAddress& targetAddress)
	{
		if (_isConnected.load() == true || _isConnectPending.load() == true)
		{
			assert(false);
			return false;
		}

		_socket = NetworkUtils::CreateSocket(true);
		if (_socket == INVALID_SOCKET)
		{
			_serverCore->HandleError(__FUNCTION__, __LINE__, "NetworkUtils::CreateSocket() : ", ::WSAGetLastError());
			return false;
		}

		if (NetworkUtils::Bind(_socket, static_cast<uint16>(0)) == false)
		{
			_serverCore->HandleError(__FUNCTION__, __LINE__, "NetworkUtils::Bind() : ", ::WSAGetLastError());
			NetworkUtils::CloseSocket(_socket);
			return false;
		}

		if (_networkDispatcher->Register(shared_from_this()) == false)
		{
			_serverCore->HandleError(__FUNCTION__, __LINE__, "_iocpCore->Register() : ", ::WSAGetLastError());
			NetworkUtils::CloseSocket(_socket);
			return false;
		}

		_remoteAddres = targetAddress;
		_isConnectPending.store(true);

		WindowsConnectEvent* connectEvent = cnew<WindowsConnectEvent>();
		connectEvent->SetOwner(shared_from_this());

		RegisterConnect(connectEvent, targetAddress);

		return true;
	}

	void Session::Disconnect()
	{
		if (_isConnected.load() == false || _isDisconnectPosted.exchange(true))
			return;

		WindowsDisconnectEvent* disconnectEvent = cnew< WindowsDisconnectEvent>();
		disconnectEvent->SetOwner(shared_from_this());

		RegisterDisconnect(disconnectEvent);
	}

	bool Session::Send(std::shared_ptr<SendContext> sendContext)
	{
		if (_isConnected.load() == false)
			return false;

		bool shouldRegister = false;

		{
			WriteLockGuard lock(_lock);
			_sendContextQueue.push(sendContext);

			if (_isSending.exchange(true) == false)
				shouldRegister = true;
		}

		if (shouldRegister)
			RegisterSend();

		return true;
	}

	void Session::Dispatch(INetworkEvent* networkEvent, bool successed, int32 errorCode, int32 numOfBytes)
	{
		//	GQCS Failed
		if (successed == false)
		{
			HandleError(networkEvent, errorCode);
			return;
		}

		//	GQCS Successed
		const auto networkEventType = networkEvent->GetNetworkEventType();

		auto session = shared_from_this();

		if (numOfBytes == 0)
		{
			switch (networkEventType)
			{
				//	ConnectEx Successed
			case NetworkEventType::Connect:
				assert(networkEvent->GetOwner() == session);
				ProcessConnect(static_cast<WindowsConnectEvent*>(networkEvent), numOfBytes);
				break;
				//	DisconnectEx Successed
			case NetworkEventType::Disconnect:
				assert(networkEvent->GetOwner() == session);
				ProcessDisconnect(static_cast<WindowsDisconnectEvent*>(networkEvent), numOfBytes);
				break;

				//	Peer�� shutdown() or closesocket() ���� ����
			case NetworkEventType::Recv:
			case NetworkEventType::Send:
				HandleError(networkEvent, ERROR_SUCCESS);
				break;

				//	???
			default:
				HandleError(networkEvent, ERROR_INVALID_PARAMETER);
				break;
			}

			return;
		}
		else
		{
			switch (networkEventType)
			{
				//	Data Recv 
			case NetworkEventType::Recv:
				assert(networkEvent->GetOwner() == session);
				ProcessRecv(static_cast<WindowsRecvEvent*>(networkEvent), numOfBytes);
				break;

				//	Data Send
			case NetworkEventType::Send:
				assert(networkEvent->GetOwner() == session);
				ProcessSend(static_cast<WindowsSendEvent*>(networkEvent), numOfBytes);
				break;

				//	???
			default:
				HandleError(networkEvent, ERROR_INVALID_PARAMETER);
				break;
			}
		}
	}

	void Session::RegisterConnect(WindowsConnectEvent* connectEvent, NetworkAddress& targetAddress)
	{
		DWORD bytesSent = 0;
		auto success = NetworkUtils::S_ConnectEx(_socket, reinterpret_cast<const sockaddr*>(&targetAddress), sizeof(sockaddr_in),
			nullptr, 0, &bytesSent, static_cast<LPOVERLAPPED>(connectEvent));

		if (success == FALSE)
		{
			int32 errorCode = ::WSAGetLastError();
			if (errorCode != WSA_IO_PENDING)
			{
				_isConnectPending.store(false);
				_serverCore->HandleError(__FUNCTION__, __LINE__, "NetworkUtils::S_ConnectEx() : ", ::WSAGetLastError());
				CloseSocket();
				cdelete(connectEvent);
			}
		}
	}

	void Session::RegisterDisconnect(WindowsDisconnectEvent* disconnectEvent)
	{
		auto success = NetworkUtils::S_DisconnectEx(_socket, static_cast<LPOVERLAPPED>(disconnectEvent), TF_REUSE_SOCKET, 0);

		if (success == FALSE)
		{
			int32 errorCode = ::WSAGetLastError();
			if (errorCode != WSA_IO_PENDING)
			{
				_serverCore->HandleError(__FUNCTION__, __LINE__, "NetworkUtils::S_DisconnectEx() : ", ::WSAGetLastError());
				_isDisconnectPosted.store(false);
				cdelete(disconnectEvent);

				_isConnected.store(false);
				CloseSocket();
				_serverCore->RemoveSession(std::static_pointer_cast<Session>(shared_from_this()));
			}
		}
	}

	void Session::RegisterRecv()
	{
		if (_isConnected.load() == false)
			return;

		WindowsRecvEvent* recvEvent = cnew<WindowsRecvEvent>();
		recvEvent->SetOwner(shared_from_this());

		WSABUF wsaBuf;
		_streamBuffer.PrepareWSARecvWsaBuf(wsaBuf);

		DWORD numOfBytes = 0;
		DWORD flags = 0;

		auto ret = ::WSARecv(_socket, (&wsaBuf), 1, &numOfBytes, &flags, static_cast<LPOVERLAPPED>(recvEvent), nullptr);

		if (ret == SOCKET_ERROR)
		{
			int32 errorCode = ::WSAGetLastError();
			if (errorCode != WSA_IO_PENDING)
			{
				_serverCore->HandleError(__FUNCTION__, __LINE__, "WSARecv() : ", errorCode);
				cdelete(recvEvent);
				Disconnect();
			}
		}
	}

	void Session::RegisterSend()
	{
		WindowsSendEvent* sendEvent = cnew<WindowsSendEvent>();
		sendEvent->SetOwner(shared_from_this());

		if (_isConnected.load() == false)
		{
			_isSending.store(false);
			cdelete(sendEvent);
			return;
		}

		{
			WriteLockGuard lock(_lock);
			while (_sendContextQueue.empty() == false)
			{
				auto& sendBuffer = _sendContextQueue.front();
				sendEvent->GetSendContexts().push_back(sendBuffer);
				_sendContextQueue.pop();
			}

			//_sendRegisterCount.store(_sendQueue.size());
		}

		std::vector<WSABUF> wsaBufs;
		wsaBufs.reserve(sendEvent->GetSendContexts().size());
		for (auto& sendContext : sendEvent->GetSendContexts())
			wsaBufs.push_back(sendContext->wsaBuf);

		DWORD numOfBytes = 0;
		auto ret = ::WSASend(_socket, wsaBufs.data(), static_cast<DWORD>(wsaBufs.size()), &numOfBytes, 0, static_cast<LPWSAOVERLAPPED>(sendEvent), nullptr);

		if (ret == SOCKET_ERROR)
		{
			int32_t errorCode = WSAGetLastError();
			if (errorCode != WSA_IO_PENDING) 
			{
				_serverCore->HandleError(__FUNCTION__, __LINE__, "WSASend() : ", errorCode);
				sendEvent->GetSendContexts().clear();
				cdelete(sendEvent);
				_isSending.store(false);
				Disconnect();
			}
		}
	}

	//	Server에서 AcceptEx로 연결된 클라이언트 후처리
	void Session::ProcessConnect()
	{
		//	이미 연결되어 있다???
		if (_isConnected.exchange(true) == true)
			return;

		OnConnected();

		RegisterRecv();
	}

	//	Client에서 ConnectEx로 연결된 서버 후처리
	void Session::ProcessConnect(WindowsConnectEvent* connectEvent, int32 numOfBytes)
	{
		//	ConnectEx 요청없이 들어왔따??
		if (_isConnectPending.exchange(false) == false)
			return;

		int32 connectError = 0;
		int32 optLen = sizeof(connectError);
		//	ConnectEx
		if (::getsockopt(_socket, SOL_SOCKET, SO_ERROR, (char*)&connectError, &optLen) != 0)
		{
			_serverCore->HandleError(__FUNCTION__, __LINE__, "getsockopt() : ", ::WSAGetLastError());
			_isConnected.store(false);
		}

		auto session = connectEvent->GetOwnerSession();
		if (connectError == 0)
		{
			//	ConnectEx
			_isConnected.store(true);

			OnConnected();

			RegisterRecv();
		}
		else
		{
			_isConnected.store(false);
			_serverCore->HandleError(__FUNCTION__, __LINE__, "ConnectEx Failed : ", ::WSAGetLastError());
			CloseSocket();
			_serverCore->RemoveSession(session);
		}

		cdelete(connectEvent);
	}

	void Session::ProcessDisconnect(WindowsDisconnectEvent* disconnectEvent, int32 numOfBytes)
	{
		_isConnected.store(false);

		auto session = disconnectEvent->GetOwnerSession();

		OnDisconnected();

		CloseSocket();
		_serverCore->RemoveSession(session);

		cdelete(disconnectEvent);
	}

	void Session::ProcessRecv(WindowsRecvEvent* recvEvent, int32 numOfBytes)
	{
		cdelete(recvEvent);

		//	처리된 데이터 크기만큼 streamBuffer writePos 처리
		if (_streamBuffer.OnWrite(numOfBytes) == false)
		{
			//	정해진 용량 초과 -> 연결 종료 
			Disconnect();
			return;
		}

		//	패킷 파싱 처리
		while (true)
		{
			const int32 readableSize = _streamBuffer.GetReadableSize();

			//	최소한 헤더 크기만큼의 데이터가 없으면 파싱 하지 않음
			if (readableSize < sizeof(PacketHeader))
				break;

			//	헤더를 읽어서 전체 패킷 크기 확인
			PacketHeader* packetHeader = reinterpret_cast<PacketHeader*>(_streamBuffer.GetReadPos());
			const uint16 packetSize = packetHeader->size;

			//	확인한 패킷 크기가 패킷 헤더보다 작다면 -> ?? 로직에 벗어난 패킷임
			if (packetSize < sizeof(PacketHeader))
			{
				//	비정상적인 패킷 크기 연결 종료
				Disconnect();
				return;
			}

			//	패킷 하나만큼의 사이즈를 읽을 수 있다면 -> 완성된 하나의 패킷을 읽을 수 있다면
			if (readableSize < packetSize)
				break;

			//	컨텐츠 영역 ( 서버 or 클라이언트 ) 에서 해당 패킷에 대한 로직 처리
			OnRecv(_streamBuffer.GetReadPos(), numOfBytes);

			//	streamBuffer ReadPos 처리
			if (_streamBuffer.OnRead(numOfBytes) == false)
			{
				//	??? 로직상 오면 안되는 부분 연결 종료
				Disconnect();
				return;
			}
		}

		RegisterRecv();
	}

	void Session::ProcessSend(WindowsSendEvent* sendEvent, int32 numOfBytes)
	{
		sendEvent->GetSendContexts().clear();
		cdelete(sendEvent);

		OnSend();

		{
			WriteLockGuard lock(_lock);
			if (_sendContextQueue.empty() == false)
				RegisterSend();
			else
				_isSending.store(false);
		}
	}

	void Session::CloseSocket()
	{
		SOCKET socket = _socket;
		if (socket != INVALID_SOCKET)
			NetworkUtils::CloseSocket(_socket);
	}

	void Session::HandleError(INetworkEvent* networkEvent, int32 errorCode)
	{
		if (networkEvent->GetNetworkEventType() == NetworkEventType::Send)
			_isSending.store(false);

		if (errorCode != ERROR_SUCCESS)
		{
			_serverCore->HandleError(__FUNCTION__, __LINE__,
				"Session::HandleError() | SessionId: " + std::to_string(_sessionId) + " , NetworkEventType: " + std::to_string(static_cast<int32>(networkEvent->GetNetworkEventType())), errorCode);
		}

		auto session = std::static_pointer_cast<Session>(shared_from_this());

		if (_isConnected.load() == true)
			Disconnect();
		else
		{
			_isConnectPending.store(false);
			CloseSocket();

			_serverCore->RemoveSession(session);
		}

		cdelete(networkEvent);
	}
#elif defined(PLATFORM_LINUX)
	std::atomic<uint64> Session::S_GenerateSessionId = 1;

	Session::Session()
	{
		_sessionId = S_GenerateSessionId.fetch_add(1);
	}

	Session::~Session()
	{
		//	TEST
		std::cout << "~Session" << std::endl;

		CloseSocket();
		
		//	TEMP
		while (_sendContextQueue.empty() == false)
			_sendContextQueue.pop();
	}

	bool Session::Connect(NetworkAddress& targetAddress)
	{
		if (_isConnected.load() == true || _isConnectPending.load() == true)
		{
			assert(false);
			return false;
		}

		_socket = NetworkUtils::CreateSocket(true);
		if (_socket == INVALID_SOCKET)
		{
			return false;
		}

		if (NetworkUtils::Bind(_socket, static_cast<uint16>(0)) == false)
		{
			NetworkUtils::CloseSocket(_socket);
			return false;
		}

		if (_networkDispatcher->Register(shared_from_this()) == false)
		{
			NetworkUtils::CloseSocket(_socket);
			return false;
		}

		struct sockaddr_in serverAddress = targetAddress.GetSocketAddress();
		_isConnectPending.store(true);

		int32 error = ::connect(_socket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
		if(error < 0)
		{
			//	EINPROGRESS 경우 연결이 즉시 완료되지 않았지만, 백그라운드에서 진행 중
			// 	이 경우 이후 epoll 등으로 EPOLLOUT 이벤트를 기다려 연결 완료를 감지
			//	그 외 음수 값 (다른 errno): 연결 시도가 명백한 오류로 실패
			if(errno != EINPROGRESS)
			{
				NetworkUtils::CloseSocket(_socket);
				_isConnectPending.store(false);
				return false;
			}
		}
		else
			//	바로 Connect
			ProcessConnect();

		return true;
	}

	void Session::Disconnect()
	{
		if (_isConnected.load() == false || _isDisconnectPosted.exchange(true))
			return;

		ProcessDisconnect();
	}

	bool Session::Send(std::shared_ptr<SendContext> sendContext)
	{
	
	}

	void Session::ProcessConnect()
	{
		int32 error = 0;
		socklen_t len = sizeof(error);
		if(::getsockopt(_socket, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0)
		{
			CloseSocket();
			_isConnectPending.store(false);
			return;
		}

		_isConnected.store(true);
		_isConnectPending.store(false);

		struct sockaddr_in remoteAddress;
		socklen_t addrLen = sizeof(remoteAddress);
		if(::getpeername(_socket, (struct sockaddr*)&remoteAddress, &addrLen) == 0)
			_remoteAddress = NetworkAddress(remoteAddress);

		OnConnected();
	}

	void Session::ProcessDisconnect()
	{
		_isConnected.store(false);

		OnDisconnected();

		CloseSocket();
		// _serverCore->RemoveSession(session);
	}

	void Session::ProcessRecv(int32 numOfBytes)
	{

	}

	void Session::ProcessSend(int32 numOfBytes)
	{

	}

	void Session::CloseSocket()
	{
		auto linuxEpollDispatcher = std::static_pointer_cast<LinuxEpollDispatcher>(_networkDispatcher);
		if(linuxEpollDispatcher->UnRegister(shared_from_this()) == false)
			;	//	???

		SOCKET socket = _socket;
		if (socket != INVALID_SOCKET)
			NetworkUtils::CloseSocket(_socket);
	}

	void Session::Dispatch(INetworkEvent* networkEvent, bool succeeded, int32 errorCode, int32 numOfBytes)
	{
		
	}

#endif
}