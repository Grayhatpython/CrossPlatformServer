#pragma once

namespace servercore
{
	struct Allocator
	{
		static void*	Allocate(size_t size);
		static void     Deallocate(void* memory);
	};

	template<typename T>
	class StlAllocator
	{
	public:
		using value_type = T;

		StlAllocator() {}

		template<typename Other>
		StlAllocator(const StlAllocator<Other>&) {}

		T* allocate(size_t count)
		{
			const uint32 size = static_cast<uint32>(count * sizeof(T));
			return static_cast<T*>(Allocator::Allocate(size));
		}

		void deallocate(T* memory, size_t count)
		{
			Allocator::Deallocate(memory);
		}
	};
}