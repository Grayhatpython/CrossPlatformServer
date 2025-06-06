#pragma once
#include "NetworkInterface.hpp"

namespace servercore
{
#if defined(PLATFORM_WINDOWS)
	class INetworkDispatcher;
	class ServerCore;

	class Acceptor : public WindowsIocpObject
	{
		friend class ServerCore;

	public:
		Acceptor();
		virtual ~Acceptor() override;

	public:
		bool	Start(uint16 port, int32 backlog = SOMAXCONN);
		void	Stop();

	public:
		virtual void	Dispatch(INetworkEvent* networkEvent, bool succeeded, int32 errorCode, int32 numOfBytes) override;
		virtual HANDLE	GetHandle() override { return reinterpret_cast<HANDLE>(_listenSocket); }

	private:
		void	RegisterAccept();
		void	ProcessAccept(WindowsAcceptEvent* acceptEvent);

	public:
		void						SetServerCore(std::shared_ptr<ServerCore> serverCore) { _serverCore = serverCore; }
		std::shared_ptr<ServerCore>	GetServerCore() { return _serverCore; }
		void						SetNetworkDispatcher(std::shared_ptr<INetworkDispatcher> networkDispatcher) { _networkDispatcher = networkDispatcher; }
		std::shared_ptr<INetworkDispatcher>	GetNetworkDispatcher() { return _networkDispatcher; }

	private:
		SOCKET									_listenSocket = INVALID_SOCKET;

		std::shared_ptr<INetworkDispatcher>		_networkDispatcher;
		std::shared_ptr<ServerCore>				_serverCore;

		std::atomic<int32>						_pendingAccepts = 0;
		std::atomic<bool>						_isClosed = false;

			//	TODO
		int32									_concurrentAcceptCount = 1;
	};
#elif defined(PLATFORM_LINUX)
	class ServerCore;
	class INetworkDispatcher;

	class Acceptor : public LinuxEpollObject
	{
	public:
		Acceptor();
		virtual ~Acceptor() override;

	public:
		bool Start(uint16 port, int32 backlog = SOMAXCONN);
		void Stop();

	public:
		virtual NetworkObjectType GetNetworkObjectType() const override { return NetworkObjectType::Acceptor; }
		virtual FileDescriptor GetFileDescriptor() override { return _listenSocket; } 
		virtual void Dispatch(INetworkEvent* networkEvent, bool succeeded, int32 errorCode) override;

	private:
		void ProcessAccept(LinuxAcceptorEvent* acceptEvent);

	public:
		void									SetServerCore(std::shared_ptr<ServerCore> serverCore) { _serverCore = serverCore; }
		std::shared_ptr<ServerCore>				GetServerCore() { return _serverCore; }
		void									SetNetworkDispatcher(std::shared_ptr<INetworkDispatcher> networkDispatcher) { _networkDispatcher = networkDispatcher; }
		std::shared_ptr<INetworkDispatcher>		GetNetworkDispatcher() { return _networkDispatcher; }

	private:
		SOCKET									_listenSocket = INVALID_SOCKET;
		std::shared_ptr<ServerCore> 			_serverCore;
		std::shared_ptr<INetworkDispatcher> 	_networkDispatcher;
	};
#endif
}