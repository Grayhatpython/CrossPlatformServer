#pragma once

namespace ServerCore
{
	enum class TaskStatus
	{
		Created,
		Running,
		Completed,
		Canceled,
	};

	class Task
	{
		using CallbackFunc = std::function<void()>;

	public:
		Task(CallbackFunc func, const std::string& name = "");
		virtual ~Task() = default;

	public:
		void Execute();
		bool Cancel();

	public:
		const std::string& GetName() const { return _name; }
		TaskStatus GetStatus() const { return _status.load(); }
		bool IsDone() const 
		{
			TaskStatus status = _status.load();
			return status == TaskStatus::Completed ||
				status == TaskStatus::Canceled;

		}

	private:
		std::string					_name;
		CallbackFunc				_func;
		std::atomic<TaskStatus>		_status = TaskStatus::Created;
		std::atomic<bool>			_cancelRequested = false;
	};

	class TaskQueue
	{
	public:
		void Push(std::shared_ptr<Task> task);
		std::shared_ptr<Task> Pop();

		bool IsEmpty() const;
		void Clear();
		void NotifyOne();
		void NotifyAll();
		void Shutdown();

		size_t Size() const;

	private:
		mutable std::mutex _lock;
		std::queue<std::shared_ptr<Task>> _tasks;
		std::condition_variable _cv;
		bool _shutdown = false;
	};

	class ThreadManager
	{
	public:
		ThreadManager(int32 threadCount = 0);
		~ThreadManager();

	public:
		void Launch(std::function<void(void)> callback, const std::string& threadName = "", bool repeat = true);
		void Join();
		void Close();

	public:
		void InitializeThreadPool(int32 threadCount = 0);
		void ShutdownThreadPool();

		//	Push Task No return Value
		std::shared_ptr<Task> PushTask(std::function<void(void)> func, const std::string& name);

	public:
		static void InitializeThreadLocal();
		static void DestroyThreadLocal();

	private:
		void WorkerThread();

	private:
		std::mutex _lock;
		std::vector<std::thread> _threads;
		std::atomic<bool> _stopped = false;

		std::vector<std::thread> _threadPool;
		std::shared_ptr<TaskQueue> _taskQueue;
		std::atomic<bool> _poolRunning = false;
	};
}