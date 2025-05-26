#pragma once

namespace ServerCore 
{
	#define MEMORY_ALIGN_SIZE 16

	struct alignas(MEMORY_ALIGN_SIZE) MemoryBlockHeader
	{
		MemoryBlockHeader(size_t allocSize, size_t dataSize)
			: allocSize(allocSize), dataSize(dataSize)
		{

		}

		static void* GetDataPos(MemoryBlockHeader* header)
		{
			return reinterpret_cast<void*>(++header);
		}

		static MemoryBlockHeader* GetHeaderPos(void* memory)
		{
			return reinterpret_cast<MemoryBlockHeader*>(memory) - 1;
		}

		size_t allocSize = 0;
		size_t dataSize = 0;
	};

	class ThreadLocalCache
	{
		friend class MemoryPool;

		static constexpr size_t S_CACHE_BUCKET_SIZE = 32;
		static constexpr size_t S_MAX_CACHE_BLOCK_SIZE = 500;

		//	...
		static constexpr size_t S_MAX_BLOCK_SIZE = 4096;
		static constexpr size_t S_MIN_BLOCK_SIZE = 32;

		struct Bucket
		{
			void* blocks[S_MAX_CACHE_BLOCK_SIZE];
			size_t count = 0;
		};

	public:
		ThreadLocalCache();
		~ThreadLocalCache();

	public:
		void*	Allocate(size_t size);
		bool	Deallocate(MemoryBlockHeader* memoryBlockHeader);
		void	RefillBlockCache(size_t bucketIndex, size_t dataSize, size_t count);

	private:
		static size_t GetBucketIndexFromSize(size_t size);
		static size_t GetSizeFromBucketIndex(size_t index);

	private:
		std::array<Bucket, S_CACHE_BUCKET_SIZE> _buckets;
	};

	class MemoryPool
	{
		static constexpr size_t S_MAX_FREELIST_COUNT = 32;
		static constexpr size_t S_REFILL_BLOCK_CACHE_COUNT = 100;

		struct FreeList
		{
			std::vector<void*> lists;
		};

	public:
		void*	Allocate(size_t dataSize);
		void	Deallocate(void* ptr);
		void	Clear();

	public:
		template<typename T, typename... Args>
		T* New(Args&&... args);

		template<typename T>
		void Delete(T* ptr);

		template<typename T, typename... Args>
		std::shared_ptr<T> MakeShared(Args&&... args);

	public:
		void*	AllocateFromMemoryPool(size_t dataSize);
		void	AllocateFromMemoryPool(size_t dataSize, size_t refillCount, std::vector<void*>& memoryBlocks);
		void	DeallocateToMemoryPool(void* memory);

	public:
		static void ThreadLocalCacheClear();

	public:
		static ThreadLocalCache& GetThreadLocalCache();
		static size_t GetBucketIndexFromThreadLocalCache(size_t size);


	private:
		void*	AllocateInternal(size_t dataSize);
		void	DeallocateInternal(void* memory);
		void*	AllocateNewMemory(size_t dataSize);

	private:
		std::array<FreeList, S_MAX_FREELIST_COUNT>		_freeLists;
		std::array<std::mutex, S_MAX_FREELIST_COUNT>	_locks;
	};

	template<typename T, typename ...Args>
	inline T* MemoryPool::New(Args && ...args)
	{
		void* memory = AllocateInternal(sizeof(T));
		return new(memory)T(std::forward<Args>(args)...);
	}

	template<typename T>
	inline void MemoryPool::Delete(T* ptr)
	{
		if (ptr)
		{
			ptr->~T();
			DeallocateInternal(static_cast<void*>(ptr));
		}
	}

	template<typename T, typename ...Args>
	inline std::shared_ptr<T> MemoryPool::MakeShared(Args && ...args)
	{
		T* ptr = New<T>(std::forward<Args>(args)...);
		return std::shared_ptr<T>(ptr, [this](T* p) { Delete(p); });
	}

	template <typename T>
	void cdelete(T* ptr) {
		GMemoryPool->Delete(ptr);
	}

	template <typename T, typename... Args>
	T* cnew(Args&&... args) {
		return GMemoryPool->New<T>(std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	std::shared_ptr<T> MakeShared(Args&&... args) {
		return GMemoryPool->MakeShared<T>(std::forward<Args>(args)...);
	}
}