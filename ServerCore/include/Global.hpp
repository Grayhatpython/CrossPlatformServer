#pragma once


namespace servercore
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

