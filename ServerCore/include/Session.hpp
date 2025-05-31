#pragma once
#include "StreamBuffer.hpp"

namespace servercore
{
	class IocpCore;
	class ServerCore;

	class Session : public IocpObject
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
		virtual void Dispatch(NetworkEvent* networkEvent, bool successed, int32 errorCode, int32 numOfBytes) override;

	public:
		virtual		void OnConnected() {};
		virtual		void OnDisconnected() {};
		virtual		void OnRecv(BYTE* buffer, int32 numOfBytes) {};
		virtual		void OnSend() {};

	private:
		void		RegisterConnect(ConnectEvent* connectEvent, NetworkAddress& targetAddress);
		void		RegisterDisconnect(DisconnectEvent* disconnectEvent);
		void		RegisterRecv();
		void		RegisterSend();

		void		ProcessConnect();
		void		ProcessConnect(ConnectEvent* connectEvent, int32 numOfBytes);
		void		ProcessDisconnect(DisconnectEvent* disconnectEvent, int32 numOfBytes);
		void		ProcessRecv(RecvEvent* recvEvent, int32 numOfBytes);
		void		ProcessSend(SendEvent* sendEvent, int32 numOfBytes);

		void		CloseSocket();

		void		HandleError(NetworkEvent* networkEvent, int32 errorCode);

	public:
		void						SetServerCore(std::shared_ptr<ServerCore> serverCore) { _serverCore = serverCore; }
		std::shared_ptr<ServerCore>	GetServerCore() { return _serverCore; }
		void						SetIocpCore(std::shared_ptr<IocpCore> iocpCore) { _iocpCore = iocpCore; }
		std::shared_ptr<IocpCore>	GetIocpCore() { return _iocpCore; }

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
		std::shared_ptr<IocpCore> _iocpCore;
		std::shared_ptr<ServerCore> _serverCore;

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