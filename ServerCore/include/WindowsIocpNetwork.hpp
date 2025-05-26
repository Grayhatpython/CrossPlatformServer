#pragma once

namespace ServerCore
{
#if defined(PLATFORM_WINDOWS)
    // 비동기 I/O 작업의 상태와 결과를 담는 OVERLAPPED 구조체를 확장한 기본 클래스
	// 각 네트워크 이벤트(Connect, Accept, Send, Recv 등)의 종류를 식별
	class NetworkEvent : public OVERLAPPED
	{
	public:
		NetworkEvent(NetworkEventType type);

	public:
		void Initialize();

	public:
		NetworkEventType GetNetworkEventType() const { return _type; }

	protected:
		NetworkEventType _type = NetworkEventType::None;
	};

	class ConnectEvent : public NetworkEvent
	{
	public:
		ConnectEvent()
			: NetworkEvent(NetworkEventType::Connect)
		{

		}
	};

	class AcceptEvent : public NetworkEvent
	{
	public:
		AcceptEvent()
			: NetworkEvent(NetworkEventType::Accept)
		{

		}
	};

	class SendEvent : public NetworkEvent
	{
	public:
		SendEvent()
			: NetworkEvent(NetworkEventType::Send)
		{

		}
	};

	class RecvEvent : public NetworkEvent
	{
	public:
		RecvEvent()
			: NetworkEvent(NetworkEventType::Recv)
		{

		}
	};

    // IOCP에 등록될 수 있는 객체를 위한 기본 인터페이스 클래스
	// 주로 Session 같은 클래스가 이를 상속받아 사용
	class IocpObject : public std::enable_shared_from_this<IocpObject>
	{
	public:
        // IocpObject가 사용하는 핸들(예: 소켓 핸들)을 반환
		virtual HANDLE GetHandle() abstract;
        // I/O 작업 완료 시 호출될 Dispatch 함수. 완료된 작업의 정보(NetworkEvent, numOfBytes)를 받아 처리
		virtual void Dispatch(class NetworkEvent* networkEvent, int32 numOfBytes = 0) abstract;
        virtual ~IocpObject() = default;
	};

    // IOCP(I/O Completion Port) 커널 객체를 관리하는 클래스
	class Iocp
	{
	public:
		Iocp();
		~Iocp();

	public:
        // 특정 IocpObject(예: 소켓)를 이 IOCP 핸들에 등록
		// 등록된 핸들에서 발생하는 I/O 완료 이벤트는 이 IOCP를 통해 처리
		bool Register(IocpObject* iocpObject = nullptr);

        // IOCP에서 완료된 I/O 이벤트를 가져와서 해당 IocpObject의 Dispatch 함수를 호출
		// timeoutMs 동안 이벤트가 없으면 타임아웃 처리
		bool Dispatch(uint32 timeoutMs = INFINITE);

	public:
		HANDLE GetHandle() { return _iocpHandle; }

	private:
		HANDLE _iocpHandle = INVALID_HANDLE_VALUE;
	};
#endif
}