#include "Pch.hpp"
#include "WindowsIocpNetwork.hpp"
#include "Session.hpp"
#include "Acceptor.hpp"

namespace servercore
{
#if defined(PLATFORM_WINDOWS)
	NetworkEvent::NetworkEvent(NetworkEventType type)
		: _type(type)
	{
		Initialize();
	}

    // OVERLAPPED 구조체 멤버 초기화
	void NetworkEvent::Initialize()
	{
		this->Internal = 0;
		this->InternalHigh = 0;
		this->Offset = 0;
		this->OffsetHigh = 0;
		this->hEvent = 0;
	}

	std::shared_ptr<Session> ConnectEvent::GetOwnerSession()
	{
		return std::static_pointer_cast<Session>(_owner);
	}

	std::shared_ptr<Session> DisconnectEvent::GetOwnerSession()
	{
		return std::static_pointer_cast<Session>(_owner);
	}

	std::shared_ptr<Acceptor> AcceptEvent::GetOwnerAcceptor()
	{
		return std::static_pointer_cast<Acceptor>(_owner);
	}

	std::shared_ptr<Session> SendEvent::GetOwnerSession()
	{
		return std::static_pointer_cast<Session>(_owner);
	}

	std::shared_ptr<Session> RecvEvent::GetOwnerSession()
	{
		return std::static_pointer_cast<Session>(_owner);
	}

	IocpCore::IocpCore()
	{
        // 새로운 I/O Completion Port를 생성
		_iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, NULL);
		assert(_iocpHandle != INVALID_HANDLE_VALUE);
	}

	IocpCore::~IocpCore()
	{
		if (_iocpHandle != INVALID_HANDLE_VALUE)
		{
			::CloseHandle(_iocpHandle);
			_iocpHandle = INVALID_HANDLE_VALUE;
		}
	}

    // IocpObject를 IOCP에 등록
	bool IocpCore::Register(std::shared_ptr<IocpObject> iocpObject)
	{
        // 기존 IOCP(_iocpHandle)에 iocpObject의 핸들(예: 소켓)을 연결
        // iocpObject 포인터 자체를 완료 키(CompletionKey)로 사용
        // I/O 완료 시 어떤 IocpObject에서 이벤트가 발생했는지 알 수 있다.
		return ::CreateIoCompletionPort(iocpObject->GetHandle(), _iocpHandle, 0, 0);
	}

	IocpGQCSResult IocpCore::Dispatch(uint32 timeoutMs)
	{
        // 전송된 바이트 수
		DWORD numOfBytes = 0;
		ULONG_PTR key;
        // 완료 키로 전달된 IocpObject
		std::shared_ptr<IocpObject> iocpObject = nullptr;
        // 완료된 OVERLAPPED 작업 구조체 
		NetworkEvent* networkEvent = nullptr;

		assert(_iocpHandle != INVALID_HANDLE_VALUE);

		// IOCP 큐에서 완료된 I/O 작업 정보를 가져온다.
		if (::GetQueuedCompletionStatus(_iocpHandle, &numOfBytes, &key,
			reinterpret_cast<LPOVERLAPPED*>(&networkEvent), timeoutMs) == TRUE)
		{
			if (key == GQCS_EXIT_SIGNAL_KEY && networkEvent == nullptr)
				return IocpGQCSResult::Exit;

			iocpObject = networkEvent->GetOwner();
			assert(iocpObject);

            // 해당 IocpObject의 Dispatch 함수를 호출하여 실제 작업 처리
			iocpObject->Dispatch(networkEvent, true, ERROR_SUCCESS, numOfBytes);
		    return IocpGQCSResult::Success;
		}
		else
		{
			int32 errorCode = ::WSAGetLastError();
			
            if(networkEvent != nullptr)
            {
				// networkEvent가 NULL이 아니라면, I/O 작업 자체는 완료되었으나 오류가 발생한 것
				// iocpObject->Dispatch를 호출하여 객체가 스스로 오류를 처리
				// numOfBytes 0으로 전달, Dispatch 내부에서 GetLastError() 등으로 구체적인 오류 확인.
                // TODO -> 정상적인지 아니면 오류로 인한건지 구분 필요

                // Iocp::Dispatch 내에서 GQCS 성공 시
                // iocpObject->Dispatch(networkEvent, numOfBytes, true, 0);

                // Iocp::Dispatch 내에서 GQCS 실패 시 (networkEvent != nullptr)
                // int specific_error = ::WSAGetLastError(); // 여기서 에러코드 캡처
                // iocpObject->Dispatch(networkEvent, 0, false, specific_error);
				iocpObject = networkEvent->GetOwner();
				assert(iocpObject);
                iocpObject->Dispatch(networkEvent, false, ::WSAGetLastError(), numOfBytes);
                return IocpGQCSResult::IoError;
            }
            else
            {
				if(key == GQCS_EXIT_SIGNAL_KEY)
					return IocpGQCSResult::Exit;

                if (errorCode == WAIT_TIMEOUT)
					return IocpGQCSResult::Timeout; // 정상적인 타임아웃
                else
                {
                    // IOCP 자체의 심각한 오류
                    // TODO
                    return IocpGQCSResult::CriticalError;
                }
            }
		}
	}
	void IocpCore::PostExitSignal()
	{
		assert(_iocpHandle != INVALID_HANDLE_VALUE);
		::PostQueuedCompletionStatus(_iocpHandle, NULL, GQCS_EXIT_SIGNAL_KEY, nullptr);
	}
#endif


}