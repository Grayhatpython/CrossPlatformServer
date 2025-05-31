#include "Pch.hpp"
#include "SendBuffer.hpp"

namespace servercore
{
    SendBuffer::SendBuffer(int32 capacity)
    {
        _buffer.resize(capacity);
    }

    BYTE* SendBuffer::Allocate(int32 size)
    {
        if (CanUseSize(size) == false)
            return nullptr;

        BYTE* allocatedPtr = _buffer.data() + _usedSize;
        _usedSize += size;
        return allocatedPtr;
    }
}


