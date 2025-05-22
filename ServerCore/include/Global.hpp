#pragma once


namespace ServerCore
{
	class CoreGlobal
	{
	public:
		CoreGlobal();
		~CoreGlobal();

	private:
		void Initialize();
		void Clear();
	};

	extern class MemoryPool* GMemoryPool;
	extern class ThreadManager* GThreadManager;
}

