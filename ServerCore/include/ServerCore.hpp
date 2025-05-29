#pragma once	

namespace servercore
{
	class Session;
	class Acceptor;
	class IocpCore;

	class ServerCore
	{
	public:
		ServerCore(uint16 port, int32 workerThreadCount);
		virtual ~ServerCore();

	public:
		bool Start();
		void Stop();

	public:
		virtual void OnSessionConnected(std::shared_ptr<Session> session);
		virtual void OnSessionDisconnected(std::shared_ptr<Session> session);
		virtual void OnSessionRecv(std::shared_ptr<Session> session, const BYTE* data, int32 length);
		
		virtual void OnClientSessionConnected(std::shared_ptr<Session> session);
		virtual void OnClientSessionDisconnected(std::shared_ptr<Session> session, int32 errorCode);

		std::shared_ptr<Session> CreateClientSessionAndConnect(NetworkAddress& targetAddress);

	public:
		virtual void HandleError(const char* func, int32 lineNumber, const std::string& message, int32 errorCode);

	private:
		void IocpWorkerThread();

	public:
		void AddSession(std::shared_ptr<Session> session);
		void RemoveSession(std::shared_ptr<Session> session);

	public:
		std::shared_ptr<IocpCore>		GetIocpCore() { return _iocpCore; }
		
	private:
		CoreGlobal*										_core;

		NetworkAddress									_listenNetworkAddress;
		std::shared_ptr<IocpCore>						_iocpCore;
		std::shared_ptr<Acceptor>						_acceptor;

		std::unordered_set<std::shared_ptr<Session>>	_sessions;
		Lock											_lock;

		std::mutex										_mutex;
		std::condition_variable							_cv;

		std::atomic<bool>								_isRunning = false;
		std::vector<std::thread>						_iocpWorkerThreads;
		int32											_workerThreadCount = 0;
	};
}