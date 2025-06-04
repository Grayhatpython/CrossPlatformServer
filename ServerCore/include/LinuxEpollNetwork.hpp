#pragma once	
#include "NetworkInterface.hpp"

namespace servercore
{
#if defined(PLATFORM_LINUX)
    class Sesssion;
    class Acceptor;
    class LinuxNetworkEvent : public INetworkEvent
    {
    public:
        LinuxNetworkEvent(NetworkEventType type)
            : INetworkEvent(type)
        {
    
        }
        virtual ~LinuxNetworkEvent() = default;

    public:
        void Initialize()
        {
            _owner.reset();
        }
    };

    class LinuxConnectEvent : public LinuxNetworkEvent
    {
    public:
        LinuxConnectEvent()
            : LinuxNetworkEvent(NetworkEventType::Connect)
        {

        }

    public:
        std::shared_ptr<Session> GetOwnerSession();
    };

    class LinuxDisconnectEvent : public LinuxNetworkEvent
    {
    public:
        LinuxDisconnectEvent()
            : LinuxNetworkEvent(NetworkEventType::Disconnect)
        {

        }

    public:
        std::shared_ptr<Session> GetOwnerSession();
    };

    class LinuxAcceptorEvent : public LinuxNetworkEvent
    {
    public:
        LinuxAcceptorEvent()
            : LinuxNetworkEvent(NetworkEventType::Accept)
        {

        }

    public:
        void    SetAcceptSocket(SOCKET socket) { _acceptSocket = socket;}
        SOCKET& GetAcceptSocket() { return _acceptSocket; }
        std::shared_ptr<Acceptor> GetOwnerAcceptor();

    private:
        SOCKET _acceptSocket = INVALID_SOCKET;
    };

    class LinuxSendEvent : public LinuxNetworkEvent
    {
    public:
        LinuxSendEvent()
            : LinuxNetworkEvent(NetworkEventType::Send)
        {

        }

    public:
        std::shared_ptr<Session> GetOwnerSession();
        std::vector<std::shared_ptr<SendContext>>& GetSendContexts() { return _sendContexts; }
    private:
        std::vector<std::shared_ptr<SendContext>> _sendContexts;
    };

    class LinuxRecvEvent : public LinuxNetworkEvent
    {
    public:
        LinuxRecvEvent()
            : LinuxNetworkEvent(NetworkEventType::Recv)
        {

        }

    public:
        std::shared_ptr<Session> GetOwnerSession();
    };

    class LinuxEpollObject : public INetworkObject
    {
    public:
        virtual HANDLE GetHandle() = 0;
        virtual void Dispatch(INetworkEvent* networkEvent, bool succeeded, int32 errorCode, int32 numOfBytes = 0) = 0;
    };

    class LinuxEpollDispatcher : public INetworkDispatcher
    {
        //  epoll_wait 한번 호출로 받을 ( 처리할 ) 이벤트의 최대 개수
        //  epoll_wait maxevents 파라미터와 관련
        //  1회 처리 작업량

        //  epoll 인스턴스가 관리할 수 있는 소켓의 총 개수랑은 다름 -> 시스템/프로세스의 자원 한계 ( ulimit -n ) -> 동시 연결 유지 개수
        static constexpr size_t S_DEFAULT_EPOLL_EVENT_SIZE = 64;

    public:
        LinuxEpollDispatcher();
        virtual ~LinuxEpollDispatcher() override;

	public:
		virtual bool Register(std::shared_ptr<INetworkObject> networkObject) override;
		virtual DispatchResult Dispatch(uint32 timeoutMs = TIMEOUT_INFINITE) override;
		virtual void PostExitSignal() override;

    public:
        FileDescriptor GetFileDescriptor() { return _epoll; }

    private:
        FileDescriptor                  _epoll = INVALID_FILE_DESCRIPTOR_VALUE;
        std::vector<struct epoll_event> _epollEvents;
        FileDescriptor                  _exitSignalEventPipe[2] = { -1, -1};
    };

#endif
}