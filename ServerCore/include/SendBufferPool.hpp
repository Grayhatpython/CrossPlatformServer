#pragma once

namespace servercore
{
    //  기본적인 SendBuffer Pool 사이즈 
    //  부족할 경우 자동으로 생성해서 추가해넣음 -> 현재는 제한은 안둠
    constexpr int32 DEFALUT_SEND_BUFFER_POOL_SIZE = 10;

    class SendBuffer;
    class SendBufferPool
    {
    public:
        SendBufferPool(int32 poolSize = DEFALUT_SEND_BUFFER_POOL_SIZE);

    public:
        std::shared_ptr<SendBuffer> Pop();
        void Pop(std::shared_ptr<SendBuffer> sendBuffer);

    private:
        std::queue<std::shared_ptr<SendBuffer>>     _pool;
        std::mutex                                  _lock;
    };

    //  SendBuffer에서 한 조각? 부분을 담당하는 구조체
    struct SendBufferSegment
    {
        SendBufferSegment(BYTE* ptr, bool successed, std::shared_ptr<SendBuffer> sendBuffer)
            : ptr(ptr), successed(successed), sendBuffer(sendBuffer)
        {

        }

        BYTE* ptr = nullptr;
        bool successed = false;
        std::shared_ptr<SendBuffer> sendBuffer;
    };

    //  전체적인 SendBuffer와 SendBufferPool을 관리하는 클래스
    class SendBufferArena
    {
    public:
        //  여기서 각 스레드의 SendBuffer 크기가 부족하면 SendBufferPool에서 Swap해서 새로운 SendBuffer을 받아옴
        static std::shared_ptr<SendBufferSegment> Allocate(int32 size);

    public:
        static int32 GetCurrentSendBufferRemainSize();
        static int32 GetCurrentSendBufferUsedSize();
        static int32 GetCurrentSendBufferRefCount();

        static void ThreadSendBufferClear();
        static void SendBufferPoolClear();

    private:
        static void SwapSendBuffer();
    
    private:
        static thread_local std::shared_ptr<SendBuffer> S_LCurrentSendBuffer;
        static std::shared_ptr<SendBufferPool>          S_SendBufferPool;
    };
}

