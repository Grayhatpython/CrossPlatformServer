#include "Pch.hpp"
#include "Lock.hpp"

namespace ServerCore
{
	Lock::Lock()
	{
#if defined(PLATFORM_WINDOWS)
		::InitializeSRWLock(&_lock);
#else
		pthread_rwlock_init(&_lock, nullptr);
#endif
	}

	Lock::~Lock()
	{
#if !defined(PLATFORM_WINDOWS)
		pthread_rwlock_destroy(&_lock);
#endif
	}

	void Lock::WriteLock()
	{
		auto ownerThreadId = _ownerThreadId.load();

		if (ownerThreadId == LThreadId)
		{
			_nestedCount.fetch_add(1);
			return;
		}

		while (true)
		{
			uint32 expected = EMPTY_OWNER_THREAD_ID;
			if (_ownerThreadId.compare_exchange_strong(expected, LThreadId))
			{
#if defined(PLATFORM_WINDOWS)
				::AcquireSRWLockExclusive(&_lock);
#else
				pthread_rwlock_wrlock(&_lock);
#endif
				_nestedCount.fetch_add(1);
				return;
			}

			for (uint32 i = 0; i < MAX_PAUSE_SPIN_COUNT; i++)
				PAUSE();
		}
	}

	void Lock::WriteUnLock()
	{
		auto ownerThreadId = _ownerThreadId.load();

		if (ownerThreadId == LThreadId)
		{
			auto prevCount = _nestedCount.fetch_sub(1);

			if (prevCount == 1)
			{
#if defined(PLATFORM_WINDOWS)
				::ReleaseSRWLockExclusive(&_lock);
#else
				pthread_rwlock_unlock(&_lock);
#endif

				_ownerThreadId.store(EMPTY_OWNER_THREAD_ID);
			}
		}
	}
}