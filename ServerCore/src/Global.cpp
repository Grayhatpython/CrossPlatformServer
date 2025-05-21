#include "Pch.hpp"
#include "Global.hpp"
#include "MemoryPool.hpp"
#include "ThreadManager.hpp"

namespace ServerCore
{
	CoreGlobal* GCoreGlobal = new CoreGlobal();
	MemoryPool* GMemoryPool = nullptr;
	ThreadManager* GThreadManager = nullptr;

	CoreGlobal::CoreGlobal()
	{
		GMemoryPool = new MemoryPool();
		GThreadManager = new ThreadManager();
	}

	CoreGlobal::~CoreGlobal()
	{

	}


	void CoreGlobal::Clear()
	{
		if (GMemoryPool)
		{
			GMemoryPool->ThreadLocalCacheClear();
			GMemoryPool->Clear();

			delete GMemoryPool;
			GMemoryPool = nullptr;
		}

		if (GThreadManager)
		{
			delete GThreadManager;
			GThreadManager = nullptr;
		}
	}
}
