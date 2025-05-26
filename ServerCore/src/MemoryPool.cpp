#include "Pch.hpp"
#include "MemoryPool.hpp"

namespace ServerCore
{
	ThreadLocalCache::ThreadLocalCache()
	{
	}

	ThreadLocalCache::~ThreadLocalCache()
	{
	}

	void* ThreadLocalCache::Allocate(size_t dataSize)
	{
		size_t totalSize = dataSize + sizeof(MemoryBlockHeader);

		const auto bucketIndex = GetBucketIndexFromSize(totalSize);
		auto& bucket = _buckets[bucketIndex];

		if (bucket.count > 0)
		{
			void* memory = bucket.blocks[--bucket.count];
			auto memoryBlockHeader = reinterpret_cast<MemoryBlockHeader*>(memory);
			memoryBlockHeader->dataSize = dataSize;
			return memory;
		}

		return nullptr;
	}

	bool ThreadLocalCache::Deallocate(MemoryBlockHeader* memoryBlockHeader)
	{
		const auto bucketIndex = GetBucketIndexFromSize(memoryBlockHeader->allocSize);
		auto& bucket = _buckets[bucketIndex];

		if (bucket.count < S_MAX_CACHE_BLOCK_SIZE)
		{
			bucket.blocks[bucket.count++] = memoryBlockHeader;
			return true;
		}

		return false;
	}

	void ThreadLocalCache::RefillBlockCache(size_t bucketIndex, size_t dataSize, size_t count)
	{
		size_t totalSize = dataSize + sizeof(MemoryBlockHeader);
		auto& bucket = _buckets[bucketIndex];

		const auto refillCount = std::min(S_MAX_CACHE_BLOCK_SIZE - bucket.count, count);

		std::vector<void*> memoryBlocks;
		GMemoryPool->AllocateFromMemoryPool(dataSize, refillCount, memoryBlocks);

		for (auto i = 0; i < memoryBlocks.size(); i++)
		{
			void* memory = memoryBlocks[i];
			if (memory)
				bucket.blocks[bucket.count++] = memory;
			else
				break;
		}


	}

	size_t ThreadLocalCache::GetBucketIndexFromSize(size_t size)
	{
		size_t extractionNumber = std::min(std::max(size, static_cast<size_t>(S_MIN_BLOCK_SIZE)), static_cast<size_t>(S_MAX_BLOCK_SIZE));

		if (extractionNumber <= 256)
			//	32 ~ 256 -> 32
			return (extractionNumber - 1) / 32;
		else if (extractionNumber <= 1024)
			//	256 ~ 1024 -> 128
			return 8 + (extractionNumber - 256 - 1) / 128;
		else
			//	1024 ~ 4096 -> 512
			return 14 + (extractionNumber - 1024 - 1) / 512;
	}

	size_t ThreadLocalCache::GetSizeFromBucketIndex(size_t index)
	{
		if (index < 8)
			// 32 ~ 256 바이트 크기 (32바이트 단위)
			return (index + 1) * 32;
		else if (index < 14)
			 // 257 ~ 1024 바이트 크기 (128바이트 단위)
			return 256 + (index - 7) * 128;
		else
			// 1025 ~ 4096 바이트 크기 (512바이트 단위)
			return 1024 + (index - 13) * 512;
	}

	void* MemoryPool::Allocate(size_t size)
	{
		void* memory = AllocateInternal(size);
		return memory;
	}

	void MemoryPool::Deallocate(void* memory)
	{
		if (memory)
			DeallocateInternal(memory);
	}

	void MemoryPool::Clear()
	{
		for (auto& lock : _locks)
		{
			std::lock_guard<std::mutex> lockGuard(lock);

			for (auto& freeList : _freeLists)
			{
				for (auto memory : freeList.lists)
				{
					if (memory)
					{
#ifdef _WIN32
						::_aligned_free(memory);
#else // POSIX
						free(memory);
#endif
					}
				}

				freeList.lists.clear();
			}
		}
	}

	void* MemoryPool::AllocateFromMemoryPool(size_t dataSize)
	{
		size_t totalSize = dataSize + sizeof(MemoryBlockHeader);

		auto bucketIndex = GetBucketIndexFromThreadLocalCache(totalSize);

		{
			std::lock_guard<std::mutex> lock(_locks[bucketIndex]);

			FreeList& freeList = _freeLists[bucketIndex];

			if (freeList.lists.empty() == false)
			{
				auto memoryBlockHeader = static_cast<MemoryBlockHeader*>(freeList.lists.back());
				freeList.lists.pop_back();
				return memoryBlockHeader;
			}
		}

		return AllocateNewMemory(dataSize);
	}

	void MemoryPool::AllocateFromMemoryPool(size_t dataSize, size_t refillCount, std::vector<void*>& memoryBlocks)
	{
		size_t totalSize = dataSize + sizeof(MemoryBlockHeader);
		auto bucketIndex = GetBucketIndexFromThreadLocalCache(totalSize);
		memoryBlocks.reserve(refillCount);

		{
			std::lock_guard<std::mutex> lock(_locks[bucketIndex]);
			FreeList& freeList = _freeLists[bucketIndex];
			size_t currentFreeListBucketCount = freeList.lists.size();

			//	Freelist에 요청한 만큼 충분한 블록이 있는 경우
			if (currentFreeListBucketCount >= refillCount)
			{
				//	Freelist에서 refillCount 만큼 가져와서 memoryBlocks에 채운다.
				for (auto i = 0; i < refillCount; i++)
				{
					memoryBlocks.push_back(freeList.lists.back());
					freeList.lists.pop_back();
				}

				return;
			}
			//	Freelist의 블록 수가 요청한 refillCount보다 적은 경우
			else
			{
				//	남아 있는 일부라도 먼저 memoryBlocks에 채운다.
				for (auto i = 0; i < currentFreeListBucketCount; i++)
				{
					memoryBlocks.push_back(freeList.lists.back());
					freeList.lists.pop_back();
				}
			}
		}

		//	필요로 하는 MermoyBlocksCount 만큼 새로 메모리를 생성해서 memoryBlocks에 채운다
		size_t neededMemoryBlocksCount = refillCount;
		if (memoryBlocks.empty() == false)
			neededMemoryBlocksCount = refillCount - memoryBlocks.size();

		for (auto i = 0; i < neededMemoryBlocksCount; i++)
		{
			auto* newMemoryBlock = AllocateNewMemory(dataSize);
			if (newMemoryBlock == nullptr)
				continue;	//	TODO
			memoryBlocks.push_back(newMemoryBlock);
		}
	}

	void MemoryPool::DeallocateToMemoryPool(void* memory)
	{
		auto memoryBlockHeader = static_cast<MemoryBlockHeader*>(memory);
		auto bucketIndex = GetBucketIndexFromThreadLocalCache(memoryBlockHeader->allocSize);

		std::lock_guard<std::mutex> lock(_locks[bucketIndex]);

		FreeList& freeList = _freeLists[bucketIndex];
		freeList.lists.push_back(memoryBlockHeader);
	}

	void MemoryPool::ThreadLocalCacheClear()
	{
		auto& threadLocalCache = GetThreadLocalCache();
		for (size_t i = 0; i < ThreadLocalCache::S_CACHE_BUCKET_SIZE; i++)
		{
			auto& bucket = threadLocalCache._buckets[i];
			for (size_t j = 0; j < bucket.count; j++)
			{
				if (bucket.blocks[j])
				{
#ifdef _WIN32
					::_aligned_free(bucket.blocks[j]);
#else // POSIX
					free(bucket.blocks[j]);
#endif

					bucket.blocks[j] = nullptr;
				}
			}

			bucket.count = 0;
		}
	}

	ThreadLocalCache& MemoryPool::GetThreadLocalCache()
	{
		thread_local ThreadLocalCache LThreadLocalCache;
		return LThreadLocalCache;
	}

	size_t MemoryPool::GetBucketIndexFromThreadLocalCache(size_t size)
	{
		return ThreadLocalCache::GetBucketIndexFromSize(size);
	}

	void* MemoryPool::AllocateInternal(size_t dataSize)
	{
		size_t totalSize = dataSize + sizeof(MemoryBlockHeader);

		auto& threadLocalCache = GetThreadLocalCache();
		void* memory = threadLocalCache.Allocate(dataSize);

		if (memory == nullptr)
		{
			auto bucketIndex = GetBucketIndexFromThreadLocalCache(totalSize);
			threadLocalCache.RefillBlockCache(bucketIndex, dataSize, S_REFILL_BLOCK_CACHE_COUNT);

			memory = threadLocalCache.Allocate(dataSize);

			if (memory == nullptr)
				memory = AllocateNewMemory(dataSize);
		}

		return MemoryBlockHeader::GetDataPos(reinterpret_cast<MemoryBlockHeader*>(memory));
	}

	void MemoryPool::DeallocateInternal(void* memory)
	{
		if (memory == nullptr)
			return;

		auto& threadLocalCache = GetThreadLocalCache();

		MemoryBlockHeader* memoryBlockHeader = MemoryBlockHeader::GetHeaderPos(memory);

		if (threadLocalCache.Deallocate(memoryBlockHeader) == false)
			DeallocateToMemoryPool(memoryBlockHeader);
	}

	void* MemoryPool::AllocateNewMemory(size_t dataSize)
	{
		size_t totalSize = dataSize + sizeof(MemoryBlockHeader);
		
		void* memory = nullptr;
#ifdef _WIN32
		memory = ::_aligned_malloc(totalSize, MEMORY_ALIGN_SIZE);
#else
		posix_memalign(&memory, MEMORY_ALIGN_SIZE, totalSize);
#endif // _WIN32

		//	Check!!
		if(memory)
			new(memory)MemoryBlockHeader(totalSize, dataSize); // placement new

		return memory;
	}

}