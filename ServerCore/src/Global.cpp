#include "Pch.hpp"
#include "Global.hpp"
#include "MemoryPool.hpp"
#include "ThreadManager.hpp"
#include "SendBufferPool.hpp"

namespace servercore
{
	MemoryPool* GMemoryPool = nullptr;
	ThreadManager* GThreadManager = nullptr;

	CoreGlobal::CoreGlobal()
	{
		Initialize();
	}

	CoreGlobal::~CoreGlobal()
	{
		Clear();
	}

	void CoreGlobal::Initialize()
	{
		GMemoryPool = new MemoryPool();
		GThreadManager = new ThreadManager();
	}

	void CoreGlobal::Clear()
	{
		if (GThreadManager)
		{
			delete GThreadManager;
			GThreadManager = nullptr;
		}

		SendBufferArena::ThreadSendBufferClear();
		SendBufferArena::SendBufferPoolClear();

		if (GMemoryPool)
		{
			GMemoryPool->ThreadLocalCacheClear();
			GMemoryPool->Clear();

			delete GMemoryPool;
			GMemoryPool = nullptr;
		}
	}
}
