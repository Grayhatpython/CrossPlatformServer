#pragma once	


namespace servercore
{
#if defined(PLATFORM_WINDOWS)
	class Session;
	class Acceptor;
	class INetworkDispatcher;

	class ServerCore : public std::enable_shared_from_this<ServerCore>
	{
	public:
		ServerCore(int32 workerThreadCount, std::function<std::shared_ptr<Session>()> sessionFactory);
		virtual ~ServerCore();

		void Stop();

	protected:
		virtual void OnStop() {};

	public:
		static void HandleError(const char* func, int32 lineNumber, const std::string& message, int32 errorCode);

	protected:
		void StartWorkerThread();

	private:
		void IocpWorkerThread();

	public:
		void AddSession(std::shared_ptr<Session> session);
		void RemoveSession(std::shared_ptr<Session> session);
		std::shared_ptr<Session> CreateSession();

		void SetSessionFactory(std::function<std::shared_ptr<Session>()> sessionFactory) { _sessionFactory = sessionFactory; }

	public:
		std::shared_ptr<INetworkDispatcher>		GetNetworkDispatcher() { return _networkDispatcher; }
		
	protected:
		CoreGlobal*										_core;

		std::shared_ptr<INetworkDispatcher>				_networkDispatcher;

		std::unordered_set<std::shared_ptr<Session>>	_sessions;
		Lock											_lock;

		std::mutex										_mutex;
		std::condition_variable							_cv;

		std::atomic<bool>								_isRunning = false;
		std::vector<std::thread>						_iocpWorkerThreads;
		int32											_workerThreadCount = 0;

		std::function<std::shared_ptr<Session>(void)>					_sessionFactory;
	};

	class ServerService : public ServerCore
	{
	public:
		ServerService(int32 workerThreadCount, std::function<std::shared_ptr<Session>()> sessionFactory);
		virtual ~ServerService();

	public:
		bool Start(uint16 port);
		virtual void OnStop() override;

	private:
		NetworkAddress						_listenNetworkAddress;
		std::shared_ptr<Acceptor>			_acceptor;
		uint16								_port = 0;

	};

	class ClinetService : public ServerCore
	{
	public:
		ClinetService(int32 workerThreadCount, std::function<std::shared_ptr<Session>()> sessionFactory);
		virtual ~ClinetService();

	public:
		virtual void OnStop() override;

	public:
		bool Connect(NetworkAddress& targetAddress, int32 connectionCount, std::vector<std::shared_ptr<Session>>& sessions);

	};
#elif defined(PLATFORM_LINUX)
	class Session;
	class Acceptor;
	class INetworkDispatcher;

	class ServerCore : public std::enable_shared_from_this<ServerCore>
	{
	public:
		ServerCore();
		~ServerCore();

	public:
		bool Initialize();
		bool Start(NetworkAddress& listenAddress);
		void Stop();

		bool StartWorkerThread(uint32 threadCount);

		void HandleError(const char* func, int32 lineNumber, const std::string& message, int32 systemErrorCode);
		void HandleError(const char* func, int32 lineNumber, const std::string& message, ErrorCode customErrorCode);

	public:
		//	TEMP
		std::shared_ptr<Session> CreateSession();
		
		void AddSession(std::shared_ptr<Session> session);
		void RemoveSession(std::shared_ptr<Session> session);

	private:
		void WorkerThread();	

	public:
		std::shared_ptr<INetworkDispatcher> GetNetworkDispatcher() { return _networkDispatcher; }
		std::shared_ptr<Acceptor>			GetAcceptor() { return _acceptor; }

	private:
		std::shared_ptr<INetworkDispatcher> 		_networkDispatcher;
		std::shared_ptr<Acceptor>					_acceptor;

		std::vector<std::thread>					_workerThreads;
		std::atomic<bool>							_isRunning = false;

		std::map<uint64, std::shared_ptr<Session>> 	_sessions;
		Lock										_lock;
	};
#endif
}