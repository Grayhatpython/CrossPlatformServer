#include "Pch.hpp"
#include "SendBufferPool.hpp"
#include "SendBuffer.hpp"

namespace servercore
{
    thread_local std::shared_ptr<SendBuffer> SendBufferArena::S_LCurrentSendBuffer;
    std::shared_ptr<SendBufferPool>          SendBufferArena::S_SendBufferPool = std::make_shared<SendBufferPool>();

    SendBufferPool::SendBufferPool(int32 poolSize)
    {
        std::lock_guard<std::mutex> lock(_lock);
        for (auto i = 0; i < poolSize; i++)
            _pool.push(std::make_shared<SendBuffer>());
    }

    std::shared_ptr<SendBuffer> SendBufferPool::Pop()
    {
        {
            std::lock_guard<std::mutex> lock(_lock);
            if (_pool.empty() == false)
            {
                auto sendBuffer = _pool.front();
                _pool.pop();
                sendBuffer->Reset();
                return sendBuffer;
            }
        }

        return std::make_shared<SendBuffer>();
    }

    void SendBufferPool::Pop(std::shared_ptr<SendBuffer> sendBuffer)
    {
        if (sendBuffer == nullptr)
            return;

        std::lock_guard<std::mutex> lock(_lock);
        sendBuffer->Reset();
        _pool.push(sendBuffer);
    }

    std::shared_ptr<SendBufferSegment> SendBufferArena::Allocate(int32 size)
    {
        if (S_LCurrentSendBuffer == nullptr || S_LCurrentSendBuffer->CanUseSize(size) == false)
            SwapSendBuffer();

        BYTE* allocatedPtr = S_LCurrentSendBuffer->Allocate(size);
        return MakeShared<SendBufferSegment>( allocatedPtr, allocatedPtr != nullptr, S_LCurrentSendBuffer );
    }

    int32 SendBufferArena::GetCurrentSendBufferRemainSize()
    {
        return S_LCurrentSendBuffer ? S_LCurrentSendBuffer->GetRemainSize() : 0;
    }

    int32 SendBufferArena::GetCurrentSendBufferUsedSize()
    {
        return S_LCurrentSendBuffer ? S_LCurrentSendBuffer->GetUsedSize() : 0;
    }

    int32 SendBufferArena::GetCurrentSendBufferRefCount()
    {
        return S_LCurrentSendBuffer ? S_LCurrentSendBuffer.use_count() : 0;
    }

    void SendBufferArena::ThreadSendBufferClear() 
    {
        S_LCurrentSendBuffer.reset(); 
    }

    void SendBufferArena::SendBufferPoolClear() 
    { 
        S_SendBufferPool.reset(); 
    }

    void SendBufferArena::SwapSendBuffer()
    {
        //  Swap이 잘되는지 테스트 문구
        std::cout << "Swap" << std::endl;
        S_LCurrentSendBuffer = S_SendBufferPool->Pop();
    }

}
