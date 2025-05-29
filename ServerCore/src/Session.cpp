#include "Pch.hpp"
#include "Session.hpp"
#include "ServerCore.hpp"

namespace servercore
{
	std::atomic<uint64> Session::S_GenerateSessionId = 1;

	Session::Session(std::shared_ptr<IocpCore> iocpCore, ServerCore* serverCore, SOCKET socket, NetworkAddress remoteAddress)
		: _iocpCore(iocpCore), _serverCore(serverCore), _socket(socket), _remoteAddres(remoteAddress)
	{
		_sessionId = S_GenerateSessionId.fetch_add(1);
		_isConnected.store(true);

		assert(_iocpCore);
		assert(_serverCore);
		assert(_socket != INVALID_SOCKET);
	}

	Session::Session(std::shared_ptr<IocpCore> iocpCore, ServerCore* serverCore)
		: _iocpCore(iocpCore), _serverCore(serverCore)
	{
		_sessionId = S_GenerateSessionId.fetch_add(1);
		_isConnected.store(false);
		
		assert(_iocpCore);
		assert(_serverCore);
	}

	Session::~Session()
	{
		//	TEST
		std::cout << "~Session" << std::endl;

		CloseSocket();
		
		//	TEMP
		while (_sendBufferQueue.empty() == false)
			_sendBufferQueue.pop();
	}

	void Session::StartAcceptedSession()
	{
		if (_isConnected.load() == false)
			return;

		_serverCore->OnSessionConnected(std::static_pointer_cast<Session>(shared_from_this()));

		RegisterRecv();
	}

	bool Session::Connect(NetworkAddress& targetAddress)
	{
		//	이미 연결된 경우 -> 일단은 false
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

		if (_iocpCore->Register(shared_from_this()) == false)
		{
			_serverCore->HandleError(__FUNCTION__, __LINE__, "_iocpCore->Register() : ", ::WSAGetLastError());
			NetworkUtils::CloseSocket(_socket);
			return false;
		}

		_remoteAddres = targetAddress;
		_isConnectPending.store(true);

		ConnectEvent* connectEvent = cnew<ConnectEvent>();
		connectEvent->SetOwner(shared_from_this());

		RegisterConnect(connectEvent, targetAddress);

		return true;
	}

	void Session::Disconnect()
	{
		if (_isConnected.load() == false || _isDisconnectPosted.exchange(true))
			return;

		DisconnectEvent* disconnectEvent = cnew< DisconnectEvent>();
		disconnectEvent->SetOwner(shared_from_this());

		RegisterDisconnect(disconnectEvent);
	}

	bool Session::Send(const BYTE* data, int32 length)
	{
		if (_isConnected.load() == false || data == nullptr || length == 0)
			return false;

		SendEvent* sendEvent = cnew<SendEvent>();
		sendEvent->SetOwner(shared_from_this());

		auto sendBuffer = MakeShared<SendBuffer>(data, length);
		
		bool shouldRegister = false;

		{
			WriteLockGuard lock(_lock);
			_sendBufferQueue.push(sendBuffer);

			if (_isSending.exchange(true) == false)
				shouldRegister = true;
		}

		if (shouldRegister)
			RegisterSend(sendEvent);

		return true;
	}

	void Session::Dispatch(NetworkEvent* networkEvent, bool successed, int32 errorCode, int32 numOfBytes)
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
				ProcessConnect(static_cast<ConnectEvent*>(networkEvent), numOfBytes);
				break;
				//	DisconnectEx Successed
			case NetworkEventType::Disconnect:
				assert(networkEvent->GetOwner() == session);
				ProcessDisconnect(static_cast<DisconnectEvent*>(networkEvent), numOfBytes);
				break;

				//	Peer가 shutdown() or closesocket() 연결 종료
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
				ProcessRecv(static_cast<RecvEvent*>(networkEvent), numOfBytes);
				break;

				//	Data Send
			case NetworkEventType::Send:
				assert(networkEvent->GetOwner() == session);
				ProcessSend(static_cast<SendEvent*>(networkEvent), numOfBytes);
				break;

				//	???
			default:
				HandleError(networkEvent, ERROR_INVALID_PARAMETER);
				break;
			}
		}
	}

	void Session::RegisterConnect(ConnectEvent* connectEvent, NetworkAddress& targetAddress)
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
				_serverCore->OnClientSessionDisconnected(connectEvent->GetOwnerSession(), errorCode);
				_serverCore->HandleError(__FUNCTION__, __LINE__, "NetworkUtils::S_ConnectEx() : ", ::WSAGetLastError());
				CloseSocket();
				cdelete(connectEvent);
			}
		}
	}

	void Session::RegisterDisconnect(DisconnectEvent* disconnectEvent)
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
				_serverCore->OnSessionDisconnected(std::static_pointer_cast<Session>(shared_from_this()));
				CloseSocket();
				_serverCore->RemoveSession(std::static_pointer_cast<Session>(shared_from_this()));
			}
		}
	}

	void Session::RegisterRecv()
	{
		if (_isConnected.load() == false)
			return;
		sizeof(RecvEvent);
		RecvEvent* recvEvent = cnew<RecvEvent>();
		recvEvent->SetOwner(shared_from_this());

		DWORD numOfBytes = 0;
		DWORD flags = 0;

		auto ret = ::WSARecv(_socket, (&recvEvent->GetWSABuf()), 1, &numOfBytes, &flags, static_cast<LPOVERLAPPED>(recvEvent), nullptr);

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

	void Session::RegisterSend(SendEvent* sendEvent)
	{
		if (_isConnected.load() == false)
		{
			_isSending.store(false);
			cdelete(sendEvent);
			return;
		}

		{
			WriteLockGuard lock(_lock);
			while (_sendBufferQueue.empty() == false)
			{
				auto& sendBuffer = _sendBufferQueue.front();
				sendEvent->GetSendBuffers().push_back(sendBuffer);
				_sendBufferQueue.pop();
			}

			//_sendRegisterCount.store(_sendQueue.size());
		}

		//	일단은 메모리풀에 남아있긴하다.. 근데 설계를 고쳐야 할 필요가
		std::vector<WSABUF> wsaBufs;
		wsaBufs.reserve(sendEvent->GetSendBuffers().size());
		for (auto& sendBuffer : sendEvent->GetSendBuffers())
		{
			WSABUF wsaBuf;
			std::memset(&wsaBuf, 0, sizeof(wsaBuf));
			wsaBuf.buf = reinterpret_cast<CHAR*>(sendBuffer->GetBuffer());
			wsaBuf.len = sendBuffer->GetLength();
			wsaBufs.push_back(wsaBuf);
		}

		DWORD numOfBytes = 0;
		auto ret = ::WSASend(_socket, wsaBufs.data(), static_cast<DWORD>(wsaBufs.size()), &numOfBytes, 0, static_cast<LPWSAOVERLAPPED>(sendEvent), nullptr);

		if (ret == SOCKET_ERROR)
		{
			int32_t errorCode = WSAGetLastError();
			if (errorCode != WSA_IO_PENDING) 
			{
				_serverCore->HandleError(__FUNCTION__, __LINE__, "WSASend() : ", errorCode);
				sendEvent->GetSendBuffers().clear();
				cdelete(sendEvent);
				_isSending.store(false);

				{
					//	TODO
					WriteLockGuard lock(_lock);
					while (_sendBufferQueue.empty() == false)
						_sendBufferQueue.pop();
				}

				Disconnect();
			}
		}
	}

	void Session::ProcessConnect(ConnectEvent* connectEvent, int32 numOfBytes)
	{
		_isConnectPending.store(false);
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
			_serverCore->OnClientSessionConnected(session);
			RegisterRecv();
		}
		else
		{
			_isConnected.store(false);
			_serverCore->OnClientSessionDisconnected(session, ::WSAGetLastError());
			_serverCore->HandleError(__FUNCTION__, __LINE__, "ConnectEx Failed : ", ::WSAGetLastError());
			CloseSocket();
			_serverCore->RemoveSession(session);
		}

		cdelete(connectEvent);
	}

	void Session::ProcessDisconnect(DisconnectEvent* disconnectEvent, int32 numOfBytes)
	{
		_isConnected.store(false);

		auto session = disconnectEvent->GetOwnerSession();

		if (_isConnectPending.load() == true)
			_serverCore->OnClientSessionDisconnected(session, ERROR_SUCCESS);
		else
			_serverCore->OnSessionDisconnected(session);

		CloseSocket();
		_serverCore->RemoveSession(session);

		cdelete(disconnectEvent);
	}

	void Session::ProcessRecv(RecvEvent* recvEvent, int32 numOfBytes)
	{
		auto session = recvEvent->GetOwnerSession();
		_serverCore->OnSessionRecv(session, reinterpret_cast<BYTE*>(recvEvent->GetWSABuf().buf), numOfBytes);

		cdelete(recvEvent);
		RegisterRecv();
	}

	void Session::ProcessSend(SendEvent* sendEvent, int32 numOfBytes)
	{
		sendEvent->GetSendBuffers().clear();
		cdelete(sendEvent);

		{
			WriteLockGuard lock(_lock);
			if (_sendBufferQueue.empty() == false)
			{
				SendEvent* sendEvent = cnew<SendEvent>();
				sendEvent->SetOwner(shared_from_this());
				RegisterSend(sendEvent);
			}
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

	void Session::HandleError(NetworkEvent* networkEvent, int32 errorCode)
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

			//	TODO
			if (networkEvent->GetNetworkEventType() == NetworkEventType::Connect)
				_serverCore->OnClientSessionDisconnected(session, errorCode);
			else
				_serverCore->OnSessionDisconnected(session);

			_serverCore->RemoveSession(session);
		}

		cdelete(networkEvent);
	}
}