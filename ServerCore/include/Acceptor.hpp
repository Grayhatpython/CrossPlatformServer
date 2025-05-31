#pragma once
#include "WindowsIocpNetwork.hpp"

namespace servercore
{
	class IocpCore;
	class ServerCore;

	class Acceptor : public IocpObject
	{
		friend class ServerCore;

	public:
		Acceptor();
		~Acceptor() override;

	public:
		bool	Start(uint16 port, int32 backlog = SOMAXCONN);
		void	Close();

	public:
		void	Dispatch(NetworkEvent* networkEvent, bool successed, int32 errorCode, int32 numOfBytes) override;
		HANDLE	GetHandle() override { return reinterpret_cast<HANDLE>(_listenSocket); }

	private:
		void	RegisterAccept();
		void	ProcessAccept(AcceptEvent* acceptEvent);

	public:
		void						SetServerCore(std::shared_ptr<ServerCore> serverCore) { _serverCore = serverCore; }
		std::shared_ptr<ServerCore>	GetServerCore() { return _serverCore; }
		void						SetIocpCore(std::shared_ptr<IocpCore> iocpCore) { _iocpCore = iocpCore; }
		std::shared_ptr<IocpCore>	GetIocpCore() { return _iocpCore; }

	private:
		SOCKET							_listenSocket = INVALID_SOCKET;

		std::shared_ptr<IocpCore>		_iocpCore;
		std::shared_ptr<ServerCore>		_serverCore;

		std::atomic<int32>				_pendingAccepts = 0;
		std::atomic<bool>				_isClosed = false;

			//	TODO
		int32							_concurrentAcceptCount = 50;
	};
}