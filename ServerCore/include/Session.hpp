#pragma once

namespace servercore
{
	class IocpCore;
	class ServerCore;

	//	TEMP
	class SendBuffer
	{
	public:
		SendBuffer(const BYTE* data, int32 length)
			: _length(length)
		{
			std::memset(_buffer.data(), 0, 4096);
			std::memcpy(_buffer.data(), data, length);
		}

	public:
		BYTE*					GetBuffer()		{ return _buffer.data(); }
		int32					GetLength() const { return _length; }

	private:
		std::array<BYTE, 4096>	_buffer;	//	TEMP
		int32					_length = 0;
	};

	class Session : public IocpObject
	{
	public:
		Session(std::shared_ptr<IocpCore> iocpCore, ServerCore* serverCore, SOCKET socket, NetworkAddress remoteAddress);
		Session(std::shared_ptr<IocpCore> iocpCore, ServerCore* serverCore);
		~Session() override;

	public:
		void StartAcceptedSession();
		bool Connect(NetworkAddress& targetAddress);

		void Disconnect();
		bool Send(const BYTE* data, int32 length);


	public:
		virtual HANDLE GetHandle() override { return reinterpret_cast<HANDLE>(_socket); }
		virtual void Dispatch(NetworkEvent* networkEvent, bool successed, int32 errorCode, int32 numOfBytes) override;

	private:
		void		RegisterConnect(ConnectEvent* connectEvent, NetworkAddress& targetAddress);
		void		RegisterDisconnect(DisconnectEvent* disconnectEvent);
		void		RegisterRecv();
		void		RegisterSend(SendEvent* sendEvent);

		void		ProcessConnect(ConnectEvent* connectEvent, int32 numOfBytes);
		void		ProcessDisconnect(DisconnectEvent* disconnectEvent, int32 numOfBytes);
		void		ProcessRecv(RecvEvent* recvEvent, int32 numOfBytes);
		void		ProcessSend(SendEvent* sendEvent, int32 numOfBytes);

		void		CloseSocket();

		void		HandleError(NetworkEvent* networkEvent, int32 errorCode);

	public:
		SOCKET&				GetSocket() { return _socket; }
		NetworkAddress		GetRemoteAddress() const { return _remoteAddres; }
		bool				IsConnected() const { return _isConnected.load(); }
		uint64				GetSessionId() const { return _sessionId; }

	public:


	private:
		static std::atomic<uint64> S_GenerateSessionId;

		SOCKET _socket = INVALID_SOCKET;
		uint64 _sessionId = 0;

		NetworkAddress _remoteAddres{};
		std::shared_ptr<IocpCore> _iocpCore;
		ServerCore* _serverCore = nullptr;

		std::atomic<bool> _isConnected = false;
		std::atomic<bool> _isConnectPending = false;
		std::atomic<bool> _isDisconnectPosted = false;

		std::queue<std::shared_ptr<SendBuffer>>		_sendBufferQueue;
		Lock										_lock;
		std::atomic<bool>							_isSending = false;
		std::atomic<uint32>							_sendRegisterCount = 0;

	};
}