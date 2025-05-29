#pragma once
#include "WindowsIocpNetwork.hpp"

namespace servercore
{
	class IocpCore;
	class ServerCore;

	class Acceptor : public IocpObject
	{
	public:
		Acceptor(std::shared_ptr<IocpCore> iocpCore, ServerCore* serverCore);
		~Acceptor() override;

	public:
		bool	Start(NetworkAddress& listenNetworkAddress, int32 concurrentAcceptCount, int32 backlog = SOMAXCONN);
		void	Close();

	public:
		void	Dispatch(NetworkEvent* networkEvent, bool successed, int32 errorCode, int32 numOfBytes) override;
		HANDLE	GetHandle() override { return reinterpret_cast<HANDLE>(_listenSocket); }

	private:
		void	RegisterAccept();
		void	ProcessAccept(AcceptEvent* acceptEvent);

	private:
		SOCKET						_listenSocket = INVALID_SOCKET;

		std::shared_ptr<IocpCore>	_iocpCore;
		ServerCore*					_serverCore = nullptr;

		std::atomic<int32>			_pendingAccepts = 0;
		std::atomic<bool>			_isClosed = false;
	};
}