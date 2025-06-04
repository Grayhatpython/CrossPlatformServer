#pragma once
#include "StreamBuffer.hpp"
#include "NetworkInterface.hpp"

namespace servercore
{
	class IocpCore;
	class ServerCore;

	class Session : public INetworkObject
	{
		friend class Acceptor;
		friend class ServerCore;

	public:
		Session();
		~Session() override;

	public:
		bool Connect(NetworkAddress& targetAddress);

		void Disconnect();
		bool Send(std::shared_ptr<SendContext> sendContext);

	public:
		virtual HANDLE GetHandle() override { return reinterpret_cast<HANDLE>(_socket); }
		virtual void Dispatch(INetworkEvent* networkEvent, bool succeeded, int32 errorCode, int32 numOfBytes) override;

	public:
		virtual		void OnConnected() {};
		virtual		void OnDisconnected() {};
		virtual		void OnRecv(BYTE* buffer, int32 numOfBytes) {};
		virtual		void OnSend() {};

	private:
		void		RegisterConnect(WindowsConnectEvent* connectEvent, NetworkAddress& targetAddress);
		void		RegisterDisconnect(WindowsDisconnectEvent* disconnectEvent);
		void		RegisterRecv();
		void		RegisterSend();

		void		ProcessConnect();
		void		ProcessConnect(WindowsConnectEvent* connectEvent, int32 numOfBytes);
		void		ProcessDisconnect(WindowsDisconnectEvent* disconnectEvent, int32 numOfBytes);
		void		ProcessRecv(WindowsRecvEvent* recvEvent, int32 numOfBytes);
		void		ProcessSend(WindowsSendEvent* sendEvent, int32 numOfBytes);

		void		CloseSocket();

		void		HandleError(INetworkEvent* networkEvent, int32 errorCode);

	public:
		void								SetServerCore(std::shared_ptr<ServerCore> serverCore) { _serverCore = serverCore; }
		std::shared_ptr<ServerCore>			GetServerCore() { return _serverCore; }
		void								SetNetworkDispatcher(std::shared_ptr<INetworkDispatcher> networkDispatcher) { _networkDispatcher = networkDispatcher; }
		std::shared_ptr<INetworkDispatcher>	GetNetworkDispatcher() { return _networkDispatcher; }

		void				SetSocket(SOCKET socket) { _socket = socket; }
		SOCKET&				GetSocket() { return _socket; }

		void				SetRemoteAddress(const NetworkAddress& remoteAddress) { _remoteAddres = remoteAddress; }
		NetworkAddress		GetRemoteAddress() const { return _remoteAddres; }

		bool				IsConnected() const { return _isConnected.load(); }
		uint64				GetSessionId() const { return _sessionId; }

	private:
		static std::atomic<uint64> S_GenerateSessionId;

		SOCKET _socket = INVALID_SOCKET;
		uint64 _sessionId = 0;

		NetworkAddress _remoteAddres{};
		std::shared_ptr<INetworkDispatcher>  _networkDispatcher;
		std::shared_ptr<ServerCore>			 _serverCore;

		std::atomic<bool> _isConnected = false;
		std::atomic<bool> _isConnectPending = false;
		std::atomic<bool> _isDisconnectPosted = false;

		std::queue<std::shared_ptr<SendContext>>	_sendContextQueue;
		Lock										_lock;
		std::atomic<bool>							_isSending = false;
		std::atomic<uint32>							_sendRegisterCount = 0;

		StreamBuffer								_streamBuffer{};
	};
}