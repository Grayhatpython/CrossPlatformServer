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
		const auto bucketIndex = GetBucketIndexFromSize(memoryBlockHeader->dataSize);
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
			// 32~256 ����
			return (index + 1) * 32;
		else if (index < 14)
			// 256~1024 ����
			return 256 + (index - 7) * 128;
		else
			// 1024~4096 ����
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
		std::lock_guard<std::mutex> lock(_lock);

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

	void* MemoryPool::AllocateFromMemoryPool(size_t dataSize)
	{
		size_t totalSize = dataSize + sizeof(MemoryBlockHeader);

		auto bucketIndex = GetBucketIndexFromThreadLocalCache(totalSize);

		{
			std::lock_guard<std::mutex> lock(_lock);

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

		{
			std::lock_guard<std::mutex> lock(_lock);
			auto freeListCount = _freeLists[bucketIndex].lists.size();

			//	freeList 비어있는 경우 or freeList 보다 요청한 refillCount가 큰 경우
			if (freeListCount < refillCount)
			{
				//	refillCount 만큼은 freeList에 미리 만들어 놓고 -> 추가 요청을 미리 대비
				for (size_t i = 0; i < refillCount; i++)
				{
					auto newMemoryBlcok = AllocateNewMemory(dataSize);
					FreeList& freeList = _freeLists[bucketIndex];
					freeList.lists.push_back(newMemoryBlcok);
				}
			}
			//	freeList가 refillCount 보다 큰 경우
			else
			{
				//	freeList에서 refillCount 만큼 가져온다.
				for (size_t i = 0; i < refillCount; i++)
				{
					FreeList& freeList = _freeLists[bucketIndex];
					auto memoryBlockHeader = static_cast<MemoryBlockHeader*>(freeList.lists.back());
					freeList.lists.pop_back();
					memoryBlocks.push_back(memoryBlockHeader);
				}
				return;
			}
		}

		//	refillCount 만큼은 threadLocalCache에 추가하기 위해
		for (size_t i = 0; i < refillCount; i++)
		{
			auto newMemoryBlcok = AllocateNewMemory(dataSize);
			memoryBlocks.push_back(newMemoryBlcok);
		}
	}

	void MemoryPool::DeallocateToMemoryPool(void* memory)
	{
		auto memoryBlockHeader = static_cast<MemoryBlockHeader*>(memory);
		auto bucketIndex = GetBucketIndexFromThreadLocalCache(memoryBlockHeader->dataSize);

		std::lock_guard<std::mutex> lock(_lock);

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