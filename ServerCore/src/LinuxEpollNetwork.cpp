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

        //  pipe() 함수를 호출하여 두 개의 파일 디스크립터를 가진 파이프를 생성
        //  파이프는 단방향 통신을 위한 커널 객체
        //  _exitSignalEventPipe[0]은 파이프의 읽기 끝(read end)이고, _exitSignalEventPipe[1]은 파이프의 쓰기 끝(write end)
        //  디스패처 스레드를 안전하게 종료하기 위한 "종료 시그널" 메커니즘으로 사용
        //  다른 스레드에서 _exitSignalEventPipe[1]에 데이터를 쓰면, epoll_wait에서 _exitSignalEventPipe[0]에 이벤트가 발생하여 디스패처 스레드가 깨어나 종료 처리
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
        //  epoll_wait -> exitSignalEventPipe[0]에 데이터가 쓰여지면 이벤트를 감지
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

    bool LinuxEpollDispatcher::Register(std::shared_ptr<INetworkObject> networkObject)
    {
        if(networkObject == nullptr)
            return false;

        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
        ev.data.ptr = networkObject.get();

        if(::epoll_ctl(_epoll, EPOLL_CTL_ADD, networkObject->GetFileDescriptor(), &ev) < 0)
            return false;

        return true;
    }

    DispatchResult LinuxEpollDispatcher::Dispatch(uint32 timeoutMs)
    {
        int32 numOfEvents = ::epoll_wait(_epoll, _epollEvents.data(), static_cast<int32>(_epollEvents.size()), timeoutMs);      
        
        if(numOfEvents < 0)
        {
            if(errno == EINTR)
                return DispatchResult::Timeout;
            else
                return DispatchResult::CriticalError;
        }
        else if(numOfEvents == 0)
            return DispatchResult::Timeout;
        else
        {
            for(int32 i = 0; i < numOfEvents; i++)
            {   
                //  종료 신호
                if(_epollEvents[i].data.fd == _exitSignalEventPipe[0])
                {
                    char buffer[0];
                    ::read(_exitSignalEventPipe[0], buffer, sizeof(buffer));
                    return DispatchResult::Exit;
                }

                auto networkObject = static_cast<INetworkObject*>(_epollEvents[i].data.ptr);
                if(networkObject == nullptr)    //  ???
                    continue;
                
                int32 events = _epollEvents[i].events;
                NetworkObjectType networkObjectType = networkObject->GetNetworkObjectType();
                
                //  Error event
                if((events & (EPOLLERR | EPOLLHUP)) != 0)
                {
                    int32 error = 0;
                    socklen_t len = sizeof(error);
                    ::getsockopt(networkObject->GetFileDescriptor(), SOL_SOCKET, SO_ERROR, &error, &len);

                    LinuxNetworkEvent networkErrorEvent(NetworkEventType::None);
                    // networkErrorEvent.SetOwner(std::shared_ptr<INetworkObject>(networkObject, [](auto) {}));
                    // networkObject->Dispatch(&networkErrorEvent, false, error, 0);
                }
                
                //  Read event
                if((events & EPOLLIN) != 0)
                {
                    if(networkObjectType == NetworkObjectType::Acceptor)
                    {
                        //  accept
                    }
                    else
                    {
                        //  session recv
                    }
                }

                //  Write event
                if((events & EPOLLOUT) != 0)
                {
                    if(networkObjectType == NetworkObjectType::Session)
                    {
                        auto session = static_cast<Session*>(networkObject);
                        
                        if(session->IsConnectPending() == true)
                        {
                            //  connect
                        }
                        else
                        {
                            //  send
                        }
                    }
                    else
                    {
                        ;   //  ????
                    }
                }
            }
        }
        
        if(numOfEvents == static_cast<int32>(_epollEvents.size()))
            _epollEvents.resize(_epollEvents.size() * 2);

        return DispatchResult::Success;
    }

    void LinuxEpollDispatcher::PostExitSignal()
    {
        char buffer[1] = {'q'};
        ::write(_exitSignalEventPipe[1], buffer, sizeof(buffer));
    }

    bool LinuxEpollDispatcher::UnRegister(std::shared_ptr<INetworkObject> networkObject)
    {
        if (::epoll_ctl(_epoll, EPOLL_CTL_DEL, networkObject->GetFileDescriptor(), nullptr) < 0) 
            return false;

        return true;
    }

#endif
}