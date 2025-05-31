#pragma once

namespace servercore
{
    //  �⺻���� SendBuffer Pool ������ 
    //  ������ ��� �ڵ����� �����ؼ� �߰��س��� -> ����� ������ �ȵ�
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

    //  SendBuffer���� �� ����? �κ��� ����ϴ� ����ü
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

    //  ��ü���� SendBuffer�� SendBufferPool�� �����ϴ� Ŭ����
    class SendBufferArena
    {
    public:
        //  ���⼭ �� �������� SendBuffer ũ�Ⱑ �����ϸ� SendBufferPool���� Swap�ؼ� ���ο� SendBuffer�� �޾ƿ�
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

