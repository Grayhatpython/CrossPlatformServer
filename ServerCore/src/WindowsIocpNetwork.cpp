#include "Pch.hpp"
#include "WindowsIocpNetwork.hpp"

namespace ServerCore
{
#if defined(PLATFORM_WINDOWS)
	NetworkEvent::NetworkEvent(NetworkEventType type)
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

	Iocp::Iocp()
	{
        // 새로운 I/O Completion Port를 생성
		_iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, NULL);
		assert(_iocpHandle != INVALID_HANDLE_VALUE);
	}

	Iocp::~Iocp()
	{
		if (_iocpHandle != INVALID_HANDLE_VALUE)
		{
			::CloseHandle(_iocpHandle);
			_iocpHandle = INVALID_HANDLE_VALUE;
		}
	}

    // IocpObject를 IOCP에 등록
	bool Iocp::Register(IocpObject* iocpObject)
	{
        // 기존 IOCP(_iocpHandle)에 iocpObject의 핸들(예: 소켓)을 연결
        // iocpObject 포인터 자체를 완료 키(CompletionKey)로 사용
        // I/O 완료 시 어떤 IocpObject에서 이벤트가 발생했는지 알 수 있다.
		return ::CreateIoCompletionPort(iocpObject->GetHandle(), _iocpHandle, reinterpret_cast<ULONG_PTR>(iocpObject), 0);
	}

	bool Iocp::Dispatch(uint32 timeoutMs)
	{
        // 전송된 바이트 수
		DWORD numOfBytes = 0;
        // 완료 키로 전달된 IocpObject
		IocpObject* iocpObject = nullptr;
        // 완료된 OVERLAPPED 작업 구조체 
		NetworkEvent* networkEvent = nullptr;

		assert(_iocpHandle != INVALID_HANDLE_VALUE);

		// IOCP 큐에서 완료된 I/O 작업 정보를 가져온다.
		if (::GetQueuedCompletionStatus(_iocpHandle, &numOfBytes, reinterpret_cast<PULONG_PTR>(&iocpObject),
			reinterpret_cast<LPOVERLAPPED*>(&networkEvent), timeoutMs) == TRUE)
		{
			assert(iocpObject && networkEvent); 
            // 해당 IocpObject의 Dispatch 함수를 호출하여 실제 작업 처리
			iocpObject->Dispatch(networkEvent, numOfBytes);
		    return true;
		}
		else
		{
			int32 errorCode = ::WSAGetLastError();
			
            if(networkEvent != nullptr)
            {
				// networkEvent가 NULL이 아니라면, I/O 작업 자체는 완료되었으나 오류가 발생한 것
				// I/O 작업을 시도했지만 오류가 발생하여 성공적으로 전송/수신된 바이트는 없다 -> numOfBytes 0
				// Dispatch 내부에서 GetLastError() 등으로 구체적인 오류 확인.

                // Iocp::Dispatch 내에서 GQCS 성공 시
                // iocpObject->Dispatch(networkEvent, numOfBytes, true, numOfBytes);

                // Iocp::Dispatch 내에서 GQCS 실패 시 (networkEvent != nullptr)
                // int specific_error = ::WSAGetLastError(); // 여기서 에러코드 캡처
                // iocpObject->Dispatch(networkEvent, numOfBytes, false, specific_error);
                assert(iocpObject);
                iocpObject->Dispatch(networkEvent, numOfBytes);
                return true;
            }
            else
            {
                if (errorCode == WAIT_TIMEOUT)
					return false; // 정상적인 타임아웃
                else
                {
                    // IOCP 자체의 심각한 오류
                    // TODO
                    return false;
                }
            }
		}
	}
#endif
}