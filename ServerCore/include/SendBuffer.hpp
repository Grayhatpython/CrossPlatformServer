#pragma once

namespace servercore
{
    constexpr int32 SEND_BUFFER_SIZE = 65536;
    
    //  각 스레드마다 가지고 있는 SendBuffer
    //  전체 관리하는 SendBufferPool에서 부족할 경우 Swap해서 새로운 SendBuffer을 얻어옴
    class SendBuffer
    {
    public:
        SendBuffer(int32 capacity = SEND_BUFFER_SIZE);
        ~SendBuffer() = default;

    public:
        //  Size만큼의 공간을 할당받음 
        //  부족할 경우 nullptr로 리턴해서 새로운 SendBuffer을 얻어옴
        //  Thread Local이라서 Lock 제한 없음
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


