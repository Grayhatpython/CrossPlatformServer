#include "Pch.hpp"
#include "Acceptor.hpp"
#include "ServerCore.hpp"
#include "Session.hpp"
#include "WindowsIocpNetwork.hpp"


namespace servercore
{
	Acceptor::Acceptor()
	{

	}

	Acceptor::~Acceptor()
	{
		Close();
	}

	bool Acceptor::Start(uint16 port, int32 backlog)
	{
		//	listenSocket이 이미 있는 경우 -> 일단은 실패
		if (_listenSocket != INVALID_SOCKET)
		{
			assert(false);
			return false;
		}

		_isClosed.store(false);
		//	Overlapped Socket
		_listenSocket = NetworkUtils::CreateSocket(true);
		if (_listenSocket == INVALID_SOCKET)
		{
			_serverCore->HandleError(__FUNCTION__, __LINE__, "NetworkUtils::CreateSocket() : ", ::WSAGetLastError());
			return false;
		}

		if (_networkDispatcher->Register(shared_from_this()) == false)
		{
			_serverCore->HandleError(__FUNCTION__, __LINE__, "_iocpCore->Register() : ", ::WSAGetLastError());
			NetworkUtils::CloseSocket(_listenSocket);
			return false;
		}

		if (NetworkUtils::SetReuseAddress(_listenSocket, true) == false)
		{
			_serverCore->HandleError(__FUNCTION__, __LINE__, "NetworkUtils::SetReuseAddress() : ", ::WSAGetLastError());
			NetworkUtils::CloseSocket(_listenSocket);
			return false;
		}

		if (NetworkUtils::Bind(_listenSocket, port) == false)
		{
			_serverCore->HandleError(__FUNCTION__, __LINE__, "NetworkUtils::Bind() : ", ::WSAGetLastError());
			NetworkUtils::CloseSocket(_listenSocket);
			return false;
		}

		if (NetworkUtils::Listen(_listenSocket, backlog) == false)
		{
			_serverCore->HandleError(__FUNCTION__, __LINE__, "NetworkUtils::Listen() : ", ::WSAGetLastError());
			NetworkUtils::CloseSocket(_listenSocket);
			return false;
		}

		for (int32 i = 0; i < _concurrentAcceptCount; i++)
			RegisterAccept();

		return true;
	}

	void Acceptor::Close()
	{
		if (_isClosed.exchange(true) == true)
			return;

		NetworkUtils::CloseSocket(_listenSocket);
	}

	void Acceptor::Dispatch(INetworkEvent* networkEvent, bool succeeded, int32 errorCode, int32 numOfBytes)
	{
		WindowsAcceptEvent* acceptEvent = static_cast<WindowsAcceptEvent*>(networkEvent);
		assert(acceptEvent->GetNetworkEventType() == NetworkEventType::Accept);
		assert(acceptEvent->GetOwnerAcceptor().get() == this);	//	TODO

		if (_isClosed.load() == true)
		{
			NetworkUtils::CloseSocket(acceptEvent->GetAcceptSocket());
			cdelete(acceptEvent);
			_pendingAccepts.fetch_sub(1);
			return;
		}

		//	GQCS Successed 
		if (succeeded)
			ProcessAccept(acceptEvent);
		//	AcceptEx GQCS Failed
		else
			NetworkUtils::CloseSocket(acceptEvent->GetAcceptSocket());

		cdelete(acceptEvent);
		_pendingAccepts.fetch_sub(1);

		//	Repeat Register Accept
		if (_isClosed.load() == false)
			RegisterAccept();
	}

	void Acceptor::RegisterAccept()
	{
		if (_isClosed.load() == true)
			return;

		SOCKET acceptedSocket = NetworkUtils::CreateSocket(true);
		if (acceptedSocket == INVALID_SOCKET)
		{
			_serverCore->HandleError(__FUNCTION__, __LINE__, "NetworkUtils::CreateSocket() : ", ::WSAGetLastError());
			return;
		}

		WindowsAcceptEvent* acceptEvent = cnew<WindowsAcceptEvent>();
		acceptEvent->SetOwner(shared_from_this());
		acceptEvent->SetAcceptSocket(acceptedSocket);

		_pendingAccepts.fetch_add(1);

		DWORD bytesReceived = 0;
		auto success = NetworkUtils::S_AcceptEx(_listenSocket, acceptedSocket, acceptEvent->GetAddressBuffer(), 0,
			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &bytesReceived, static_cast<LPOVERLAPPED>(acceptEvent));

		if (success == FALSE)
		{
			int32 errorCode = ::WSAGetLastError();
			if (errorCode != WSA_IO_PENDING)
			{
				_serverCore->HandleError(__FUNCTION__, __LINE__, "NetworkUtils::S_AcceptEx() : ", errorCode);
				NetworkUtils::CloseSocket(acceptEvent->GetAcceptSocket());
				cdelete(acceptEvent);
				_pendingAccepts.fetch_sub(1);

				//	Repeat Register Accept
				if (_isClosed.load() == false)
					RegisterAccept();
			}
		}
	}

	void Acceptor::ProcessAccept(WindowsAcceptEvent* acceptEvent)
	{
		if (NetworkUtils::SetUpdateAcceptSocket(acceptEvent->GetAcceptSocket(), _listenSocket) == false)
		{
			_serverCore->HandleError(__FUNCTION__, __LINE__, "NetworkUtils::SetUpdateAcceptSocket() : ", ::WSAGetLastError());
			NetworkUtils::CloseSocket(acceptEvent->GetAcceptSocket());
			cdelete(acceptEvent);
			return;
		}

		struct sockaddr_in remoteSocketAddress;
		std::memset(&remoteSocketAddress, 0, sizeof(remoteSocketAddress));
		int32 addrLen = sizeof(remoteSocketAddress);

		if (::getpeername(acceptEvent->GetAcceptSocket(), reinterpret_cast<sockaddr*>(&remoteSocketAddress), &addrLen) != 0)
		{
			_serverCore->HandleError(__FUNCTION__, __LINE__, "getpeername() : ", ::WSAGetLastError());
			NetworkUtils::CloseSocket(acceptEvent->GetAcceptSocket());
			cdelete(acceptEvent);
			return;
		}

		NetworkAddress remoteAddress(remoteSocketAddress);

		auto newSession = _serverCore->CreateSession();
		assert(newSession);
		
		newSession->SetSocket(acceptEvent->GetAcceptSocket());
		newSession->SetRemoteAddress(remoteAddress);
		
		if (_networkDispatcher->Register(std::static_pointer_cast<INetworkObject>(newSession)) == false)
		{
			_serverCore->HandleError(__FUNCTION__, __LINE__, "_iocpCore->Register() : ", ::WSAGetLastError());
			cdelete(acceptEvent);
			return;
		}

		_serverCore->AddSession(newSession);

		//	TODO
		newSession->ProcessConnect();
	}
}