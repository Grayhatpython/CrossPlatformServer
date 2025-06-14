#include "Pch.hpp"
#include "ThreadManager.hpp"

namespace servercore
{
	Task::Task(CallbackFunc func, const std::string& name)
		: _name(name), _func(func)
	{

	}

	void Task::Execute()
	{
		if (_cancelRequested.load() == true)
		{
			_status.store(TaskStatus::Canceled);
			return;
		}

		_status.store(TaskStatus::Running);

		if (_func)
		{
			_func();
			_status.store(TaskStatus::Completed);
		}
	}

	bool Task::Cancel()
	{
		_cancelRequested.store(true);
		return true;
	}

	void TaskQueue::Push(std::shared_ptr<Task> task)
	{
		{
			std::lock_guard<std::mutex> lock(_lock);
			if (_shutdown)
				return;

			_tasks.push(task);
		}

		_cv.notify_one();
	}

	std::shared_ptr<Task> TaskQueue::Pop()
	{
		std::unique_lock<std::mutex> lock(_lock);
		_cv.wait(lock, [this]() { return _shutdown || _tasks.empty() == false; });

		if (_shutdown || _tasks.empty() == true)
			return nullptr;

		auto task = _tasks.front();
		_tasks.pop();
		return task;
	}

	bool TaskQueue::IsEmpty() const
	{
		std::lock_guard<std::mutex> lock(_lock);
		return _tasks.empty();
	}

	void TaskQueue::Clear()
	{
		std::lock_guard<std::mutex> lock(_lock);
		while (_tasks.empty() == false)
			_tasks.pop();
	}

	void TaskQueue::NotifyOne()
	{
		_cv.notify_one();
	}

	void TaskQueue::NotifyAll()
	{
		_cv.notify_all();
	}

	void TaskQueue::Shutdown()
	{
		{
			std::lock_guard<std::mutex> lock(_lock);
			_shutdown = true;
		}

		_cv.notify_all();
	}

	size_t TaskQueue::Size() const
	{
		std::lock_guard<std::mutex> lock(_lock);
		return _tasks.size();
	}

	ThreadManager::ThreadManager(int32 threadCount)
	{
		_taskQueue = std::make_shared<TaskQueue>();

		InitializeThreadLocal();

		if (threadCount > 0)
			InitializeThreadPool();
	}

	ThreadManager::~ThreadManager()
	{
		Close();
		Join();
		ShutdownThreadPool();
	}

	void ThreadManager::Launch(std::function<void()> callback, const std::string& threadName,  bool repeat)
	{
		if (_stopped.load() == true)
			return;

		std::lock_guard<std::mutex> lock(_lock);

		_threads.push_back(std::thread([this, callback, threadName, repeat]() {
			
			InitializeThreadLocal();

			if (repeat)
			{
				while (_stopped.load() == false)
					callback();
			}
			else
			{
				if(_stopped.load() ==false)
					callback();
			}

			DestroyThreadLocal();

			std::cout << threadName << " thread [" << LThreadId << "] finished." << std::endl;
		}));
	}

	void ThreadManager::Join()
	{
		for (std::thread& thread : _threads)
		{
			if (thread.joinable())
				thread.join();
		}
		_threads.clear();
	}

	void ThreadManager::Close()
	{
		_stopped.store(true);
		_taskQueue->NotifyAll();
	}

	void ThreadManager::InitializeThreadPool(int32 threadCount)
	{
		if (_poolRunning.load() == true)
			return;

		if (threadCount <= 0)
			threadCount = std::thread::hardware_concurrency();

		_poolRunning.store(true);

		for (int32 i = 0; i < threadCount; i++)
		{
			_threadPool.push_back(std::thread([this, i]() {
					WorkerThread();
				}));
		}
	}

	void ThreadManager::ShutdownThreadPool()
	{
		_poolRunning.store(false);
		_taskQueue->Shutdown();

		for (auto& thread : _threadPool)
		{
			if (thread.joinable())
				thread.join();
		}

		_threadPool.clear();
	}

	std::shared_ptr<Task> ThreadManager::PushTask(std::function<void(void)> func, const std::string& name)
	{
		auto task = std::make_shared<Task>(func, name);
		_taskQueue->Push(task);
		return task;
	}

	void ThreadManager::InitializeThreadLocal()
	{
		static std::atomic<uint32> S_autoIncreaseThreadId = 1;
		LThreadId = S_autoIncreaseThreadId.fetch_add(1);
	}

	void ThreadManager::DestroyThreadLocal()
	{
		SendBufferArena::ThreadSendBufferClear();
		GMemoryPool->ThreadLocalCacheClear();
	}

	void ThreadManager::WorkerThread()
	{
		InitializeThreadLocal();

		while (_poolRunning.load() == true)
		{
			auto task = _taskQueue->Pop();
			if (task)
				task->Execute();
		}

		DestroyThreadLocal();

		std::cout << "Thread Pool thread [" << LThreadId << "] finished." << std::endl;
	}
}