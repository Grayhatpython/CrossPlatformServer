#pragma once

namespace servercore
{
    constexpr int32 SEND_BUFFER_SIZE = 65536;
    
    //  �� �����帶�� ������ �ִ� SendBuffer
    //  ��ü �����ϴ� SendBufferPool���� ������ ��� Swap�ؼ� ���ο� SendBuffer�� ����
    class SendBuffer
    {
    public:
        SendBuffer(int32 capacity = SEND_BUFFER_SIZE);
        ~SendBuffer() = default;

    public:
        //  Size��ŭ�� ������ �Ҵ���� 
        //  ������ ��� nullptr�� �����ؼ� ���ο� SendBuffer�� ����
        //  Thread Local�̶� Lock ���� ����
        BYTE* Allocate(int32 size);

    public:
        void Reset() { _usedSize = 0; }
        bool IsEmpty() const { return _usedSize == 0; }
        bool CanUseSize(int32 size) const { return GetRemainSize() >= size; }

    public:
        BYTE* GetBuffer() { return _buffer.data(); }
        int32 GetCapacity() const { return static_cast<int32>(_buffer.size()); }
        int32 GetUsedSize() const noexcept { return _usedSize; }
        int32 GetRemainSize() const { return GetCapacity() - _usedSize; }

    private:
        std::vector<BYTE>   _buffer;
        int32               _usedSize = 0;
    };
}


