#include "Pch.hpp"
#include "Allocator.hpp"
#include "MemoryPool.hpp"

namespace servercore
{
	void* Allocator::Allocate(size_t size)
	{
		return GMemoryPool->Allocate(size);
	}

	void Allocator::Deallocate(void* ptr)
	{
		GMemoryPool->Deallocate(ptr);
	}
}