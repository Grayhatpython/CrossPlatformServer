#pragma once	

namespace servercore
{
	class Session;
	class Acceptor;
	class IocpCore;

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
		std::shared_ptr<IocpCore>		GetIocpCore() { return _iocpCore; }
		
	protected:
		CoreGlobal*										_core;

		std::shared_ptr<IocpCore>						_iocpCore;

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
}