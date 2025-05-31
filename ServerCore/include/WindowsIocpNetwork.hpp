#pragma once

namespace servercore
{
#if defined(PLATFORM_WINDOWS)

	class Session;
	class Acceptor;
	class IocpObject;

    // 비동기 I/O 작업의 상태와 결과를 담는 OVERLAPPED 구조체를 확장한 기본 클래스
	// 각 네트워크 이벤트(Connect, Accept, Send, Recv 등)의 종류를 식별
	class NetworkEvent : public OVERLAPPED
	{
	public:
		NetworkEvent(NetworkEventType type);

	public:
		void Initialize();

	public:
		NetworkEventType			GetNetworkEventType() const { return _type; }

		std::shared_ptr<IocpObject> GetOwner() { return _owner; }
		void						SetOwner(std::shared_ptr<IocpObject> owner) { _owner = owner; }

	protected:
		NetworkEventType			_type = NetworkEventType::None;

		//	네트워크 이벤트 주체 ( Session, Acceptor )
		std::shared_ptr<IocpObject> _owner;
	};

	class ConnectEvent : public NetworkEvent
	{
	public:
		ConnectEvent()
			: NetworkEvent(NetworkEventType::Connect)
		{

		}

	public:
		std::shared_ptr<Session> GetOwnerSession();

	};

	class DisconnectEvent : public NetworkEvent
	{
	public:
		DisconnectEvent()
			: NetworkEvent(NetworkEventType::Disconnect)
		{

		}

	public:
		std::shared_ptr<Session> GetOwnerSession();

	};

	class AcceptEvent : public NetworkEvent
	{
	public:
		AcceptEvent()
			: NetworkEvent(NetworkEventType::Accept)
		{
			std::memset(&_addressBuffer, 0, sizeof(_addressBuffer));
		}

	public:
		void	SetAcceptSocket(SOCKET socket) { _acceptSocket = socket; }
		SOCKET&	GetAcceptSocket() { return _acceptSocket; }

		char*	GetAddressBuffer() { return _addressBuffer; }

	public:
		std::shared_ptr<Acceptor> GetOwnerAcceptor();

	private:
		SOCKET	_acceptSocket = INVALID_SOCKET;
		char	_addressBuffer[(sizeof(sockaddr_in) + 16) * 2];
	};

	class SendBuffer;
	//	WSASend시 SendBuffer 즉, 요청보낸 Send data가 삭제되면 안되므로 SendBuffer의 Ref Count를 증가시켜서
	//	GQCS -> Dispatch -> ProcessSend에서 WSASend가 정상적으로 완료되었을 때 Ref Count를 0으로 만들어 제거
	//	Owner인 WSASend, WSARecv 등 IO 작업의 주체자도 물론 Io 진행시 삭제되면 안되므로 동등하게 Ref Count로 관리
	struct SendContext
	{
		std::shared_ptr<SendBuffer> sendBuffer;
		WSABUF						wsaBuf{};
	};

	class SendEvent : public NetworkEvent
	{
	public:
		SendEvent()
			: NetworkEvent(NetworkEventType::Send)
		{
		}

	public:
		std::shared_ptr<Session>					GetOwnerSession();
		std::vector<std::shared_ptr<SendContext>>&	GetSendContexts() { return _sendContexts; }

	private:
		std::vector<std::shared_ptr<SendContext>>	_sendContexts;
	};

	class RecvEvent : public NetworkEvent
	{
	public:
		RecvEvent()
			: NetworkEvent(NetworkEventType::Recv)
		{

		}

	public:
		std::shared_ptr<Session>	GetOwnerSession();

	};

    // IOCP에 등록될 수 있는 객체를 위한 기본 인터페이스 클래스
	// 주로 Session 같은 클래스가 이를 상속받아 사용
	class IocpObject : public std::enable_shared_from_this<IocpObject>
	{
	public:
		IocpObject() = default;
        virtual ~IocpObject() = default;

	public:
        // IocpObject가 사용하는 핸들(예: 소켓 핸들)을 반환
		virtual HANDLE GetHandle() abstract;
        // I/O 작업 완료 시 호출될 Dispatch 함수. 완료된 작업의 정보(NetworkEvent, numOfBytes)를 받아 처리
		virtual void Dispatch(class NetworkEvent* networkEvent, bool successed, int32 errorCode, int32 numOfBytes = 0) abstract;
	};

    // IOCP(I/O Completion Port) 커널 객체를 관리하는 클래스
	class IocpCore
	{
		const ULONG_PTR GQCS_EXIT_SIGNAL_KEY = 0xDEADBEEF;

	public:
		IocpCore();
		~IocpCore();

	public:
        // 특정 IocpObject(예: 소켓)를 이 IOCP 핸들에 등록
		// 등록된 핸들에서 발생하는 I/O 완료 이벤트는 이 IOCP를 통해 처리
		bool Register(std::shared_ptr<IocpObject> iocpObject);

        // IOCP에서 완료된 I/O 이벤트를 가져와서 해당 IocpObject의 Dispatch 함수를 호출
		// timeoutMs 동안 이벤트가 없으면 타임아웃 처리
		IocpGQCSResult	Dispatch(uint32 timeoutMs = INFINITE);
		
	public:
		void			PostExitSignal();

	public:
		HANDLE GetHandle() { return _iocpHandle; }

	private:
		HANDLE _iocpHandle = INVALID_HANDLE_VALUE;
	};
#endif
}