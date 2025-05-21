#pragma once		

#if defined(PLATFORM_WINDOWS)
#define PAUSE() YieldProcessor()
#elif defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
#define PAUSE() __asm__ __volatile__ ("pause")
#elif defined(__arm__) || defined(__aarch64__) || defined(_M_ARM64)
#define PAUSE() __asm__ __volatile__("yield")
#else
#define PAUSE() std::this_thread::yield()
#endif

namespace ServerCore
{
	class Lock
	{
		enum
		{
			EMPTY_OWNER_THREAD_ID = 0,
			MAX_PAUSE_SPIN_COUNT = 1024,
		};

	public:
		Lock();
		~Lock();

	public:
		void WriteLock();
		void WriteUnLock();

	private:
#if defined(PLATFORM_WINDOWS)
		SRWLOCK _lock;
#else
		pthread_rwlock_t _lock;
#endif

		std::atomic<uint32> _ownerThreadId = 0;
		std::atomic<uint32> _nestedCount = 0;
	};

	class WriteLockGuard
	{
	public:
		WriteLockGuard(Lock& lock)
			: _lock(lock)
		{
			_lock.WriteLock();
		}
		~WriteLockGuard()
		{
			_lock.WriteUnLock();
		}

	private:
		Lock& _lock;
	};
}