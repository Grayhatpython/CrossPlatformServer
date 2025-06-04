#pragma once

namespace servercore
{
	class StreamBuffer
	{
	public:
		StreamBuffer(int32 capacity = 65536);

	public:
		bool OnRead(int32 numOfBytes);
		bool OnWrite(int32 numOfBytes);
		void Clean();
		#if defined(PLATFORM_WINDOWS)
		void PrepareWSARecvWsaBuf(WSABUF& wsaBuf);
		#elif defined(PLATFORM_LINUX)
		#endif

	public:
		int32 GetReadableSize() const { return _writePos - _readPos; }
		int32 GetWriteableSize() const { return _capacity - _writePos; }
		BYTE* GetReadPos() { return _buffer.data() + _readPos; }
		BYTE* GetWritePos() { return _buffer.data() + _writePos; }

	private:
		std::vector<BYTE>	_buffer;
		int32				_capacity = 0;
		int32				_readPos = 0;
		int32				_writePos = 0;
	};
}