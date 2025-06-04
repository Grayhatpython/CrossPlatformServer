#include "Pch.hpp"
#include "LinuxEpollNetwork.hpp"
#include "Acceptor.hpp"
#include "Session.hpp"

namespace servercore
{
#if defined(PLATFORM_LINUX)
    std::shared_ptr<Session> LinuxConnectEvent::GetOwnerSession()
    {
        return std::static_pointer_cast<Session>(_owner);
    }

    std::shared_ptr<Session> LinuxDisconnectEvent::GetOwnerSession()
    {
        return std::static_pointer_cast<Session>(_owner);
    }
    
    std::shared_ptr<Acceptor> LinuxAcceptorEvent::GetOwnerAcceptor()
    {
        return std::static_pointer_cast<Acceptor>(_owner);
    }

    std::shared_ptr<Session> LinuxSendEvent::GetOwnerSession()
    {
        return std::static_pointer_cast<Session>(_owner);
    }

     std::shared_ptr<Session> LinuxRecvEvent::GetOwnerSession()
    {
       return std::static_pointer_cast<Session>(_owner);
    }

    LinuxEpollDispatcher::LinuxEpollDispatcher()
    {
        _epoll = ::epoll_create1(0);
        if(_epoll < 0)
            throw std::runtime_error("epoll_create1() failed : " + std::string(strerror(errno)));   

        if(::pipe(_exitSignalEventPipe) < 0)
            throw std::runtime_error("pipe() failed : " + std::string(strerror(errno)));   
        
        for(int i = 0; i < 2; i++)
        {
            if(NetworkUtils::SetNonBlocking(_exitSignalEventPipe[i]) == false)
                throw std::runtime_error("NetworkUtils::SetNonBlocking() failed : " + std::string(strerror(errno)));   
        }

        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = _exitSignalEventPipe[0];
        if(epoll_ctl(_epoll, EPOLL_CTL_ADD, _exitSignalEventPipe[0], &ev) < 0)
            throw std::runtime_error("epoll_ctl() failed : " + std::string(strerror(errno)));   

        //  TODO
        _epollEvents.resize(S_DEFAULT_EPOLL_EVENT_SIZE);
    }

    LinuxEpollDispatcher::~LinuxEpollDispatcher()
    {
        if(_epoll >= 0)
        {
            ::close(_epoll);
            _epoll = INVALID_FILE_DESCRIPTOR_VALUE;
        }

        for(int i = 0; i < 2; i++)
        {
            if(_exitSignalEventPipe[i] >= 0)
            {
                ::close(_exitSignalEventPipe[i]);
                _exitSignalEventPipe[i] = INVALID_FILE_DESCRIPTOR_VALUE;
            }
        }
    }

    bool LinuxEpollDispatcher::Register(std::shared_ptr<INetworkObject> networkObject>
    {

    }

    DispatchResult LinuxEpollDispatcher::Dispatch(uint32 timeoutMs)
    {

    }

    void LinuxEpollDispatcher::PostExitSignal()
    {

    }
#endif
}