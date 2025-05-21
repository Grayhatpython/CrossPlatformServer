#pragma once


namespace ServerCore
{
	class CoreGlobal
	{
	public:
		CoreGlobal();
		~CoreGlobal();

	public:
		void Clear();
	};

	extern CoreGlobal* GCoreGlobal;
	extern class MemoryPool* GMemoryPool;
	extern class ThreadManager* GThreadManager;
}

