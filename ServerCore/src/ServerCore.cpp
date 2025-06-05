#include "Pch.hpp"
#include "ServerCore.hpp"
#include "Acceptor.hpp"
#include "Session.hpp"

#if defined(PLATFORM_WINDOWS)
#include "WindowsIocpNetwork.hpp"
#elif defined(PLATFORM_LINUX)
#include "LinuxEpollNetwork.hpp"
#endif

namespace servercore
{
#if defined(PLATFORM_WINDOWS)
	ServerCore::ServerCore(int32 workerThreadCount, std::function<std::shared_ptr<Session>()> sessionFactory)
		: _sessionFactory(sessionFactory)
	{
		NetworkUtils::Initialize(); 

		_core = new CoreGlobal();
		_workerThreadCount = workerThreadCount > 0 ? workerThreadCount : std::thread::hardware_concurrency();
		_networkDispatcher = std::make_shared<WindowsIocpDispatcher>();
	}

    ServerCore::~ServerCore()
    {
		Stop();
		NetworkUtils::Clear();
	}

	void ServerCore::Stop()
	{
		if (_isRunning.load() == false)
			return;

		std::cout << "ServerCore stopping..." << std::endl;

		//	각 Service별 Stop Logic 실행
		OnStop();

		{
			//	session clear
			std::vector<std::shared_ptr<Session>> sessions;
			{
				WriteLockGuard lock(_lock);
				for (const auto& session : _sessions)
					sessions.push_back(session);
			}

			if (sessions.empty() == false)
			{
				std::cout << sessions.size() << " active sessionsto disconnect..." << std::endl;
				for (const auto& session : sessions)
				{
					if (session)
						session->Disconnect();
				}

				std::unique_lock<std::mutex> lock(_mutex);
				std::cout << "Waiting for all sessions to complete disconnect..." << std::endl;
				_cv.wait(lock, [this]() {
						WriteLockGuard lock(_lock);
						return _sessions.empty();
					});
				std::cout << "All sessions have completed disconnect..." << std::endl;
			}	
		}


		{
			//	thread clase
			for (auto i = 0; i < _workerThreadCount; i++)
				_networkDispatcher->PostExitSignal();

			for (std::thread& t : _iocpWorkerThreads)
			{
				if (t.joinable()) {
					t.join();
				}
			}
			_iocpWorkerThreads.clear();
		}

		{
			WriteLockGuard lock(_lock);
			if (_sessions.empty() == false) 
			{
				std::cerr << "Warning: " << _sessions.size() << " sessions still present after targeted shutdown. Forcing clear.\n";
				_sessions.clear();
			}
		}

		{
			_networkDispatcher.reset();

			if (_core)
			{
				delete _core;
				_core = nullptr;
			}
		}

		_isRunning.store(false);
		std::cout << "ServerCore stopped..." << std::endl;
	}

	void ServerCore::HandleError(const char* func, int32 lineNumber, const std::string& message, int32 errorCode)
	{
		std::cerr << "[ERROR] Func: " << func << " | Line: " << lineNumber << " | Msg: " << message << " | Code: " << errorCode << " (0x" << std::hex << errorCode << std::dec << ")\n";
	}

	void ServerCore::StartWorkerThread()
	{
		_iocpWorkerThreads.reserve(_workerThreadCount);
		for (auto i = 0; i < _workerThreadCount; i++)
			_iocpWorkerThreads.emplace_back(&ServerCore::IocpWorkerThread, this);
	}

	void ServerCore::IocpWorkerThread()
	{
		GThreadManager->InitializeThreadLocal();

		while (_isRunning.load() == true)
		{
			DispatchResult result = _networkDispatcher->Dispatch(100);

			//	CriticalError or Exit Singal 
			if (result == DispatchResult::Exit || result == DispatchResult::CriticalError)
				break;
		}

		GThreadManager->DestroyThreadLocal();

		std::cout << "Iocp Worker thread [" << LThreadId << "] finished." << std::endl;
	}

	void ServerCore::AddSession(std::shared_ptr<Session> session)
	{
		WriteLockGuard lock(_lock);
		_sessions.insert(session);
		std::cout << "Session added : " << session->GetSessionId() << ". Total: " << _sessions.size() << std::endl;
	}

	void ServerCore::RemoveSession(std::shared_ptr<Session> session)
	{
		bool notify = false;

		{
			WriteLockGuard lock(_lock);
			_sessions.erase(session);
			std::cout << "Session removed : " << session->GetSessionId() << ". Total: " << _sessions.size() << std::endl;

			if (_isRunning.load() == false && _sessions.empty())
				notify = true;
		}

		if (notify)
		{
			std::unique_lock<std::mutex> lock(_mutex);
			_cv.notify_all();
		}
	}

	std::shared_ptr<Session> ServerCore::CreateSession()
	{
		auto session = _sessionFactory();
		assert(session);

		session->SetNetworkDispatcher(_networkDispatcher);
		session->SetServerCore(shared_from_this());

		return session;
	}

	ServerService::ServerService(int32 workerThreadCount, std::function<std::shared_ptr<Session>()> sessionFactory)
		:	ServerCore(workerThreadCount, sessionFactory), _acceptor(std::make_shared<Acceptor>())
	{

	}

	ServerService::~ServerService()
	{

	}

	bool ServerService::Start(uint16 port)
	{
		if (_isRunning.exchange(true) == true)
		{
			HandleError(__FUNCTION__, __LINE__, "ServerService is already starting", ERROR_SUCCESS);
			return false;
		}

		assert(_acceptor);
		_acceptor->SetNetworkDispatcher(_networkDispatcher);
		_acceptor->SetServerCore(shared_from_this());

		//	TODO : 50
		if (_acceptor->Start(port) == false)
		{
			HandleError(__FUNCTION__, __LINE__, "ConnectEx Failed : ", ERROR_SUCCESS);
			_isRunning.store(false);
			return false;
		}

		_port = port;

		StartWorkerThread();

		std::cout << "ServerService Started." << std::endl;
		std::cout << "Listening Info : ip[ " << _listenNetworkAddress.GetIpStringAddress() << " ] " << "port[ " << _listenNetworkAddress.GetPort() << " ]" << std::endl;
		std::cout << "WorkerThread Count : " << _workerThreadCount << std::endl;

		return true;
	}

	void ServerService::OnStop()
	{
		//	acceptor clear
		if (_acceptor)
			_acceptor->Stop();	//	TODO

		//	acceptor clear wait 
		std::this_thread::sleep_for(std::chrono::seconds(1));

		_acceptor.reset();
	}

	ClinetService::ClinetService(int32 workerThreadCount, std::function<std::shared_ptr<Session>()> sessionFactory)
		: ServerCore(workerThreadCount, sessionFactory)
	{

	}

	ClinetService::~ClinetService()
	{

	}

	void ClinetService::OnStop()
	{

	}

	bool ClinetService::Connect(NetworkAddress& targetAddress, int32 connectionCount, std::vector<std::shared_ptr<Session>>& sessions)
	{
		if (_isRunning.exchange(true) == true)
		{
			HandleError(__FUNCTION__, __LINE__, "Server not running", ERROR_SUCCESS);
			return false;
		}


		for (auto i = 0; i < connectionCount; i++)
		{
			auto seession = CreateSession();
			assert(seession);

			AddSession(seession);

			if (seession->Connect(targetAddress) == false) {
				HandleError(__FUNCTION__, __LINE__, "clientSession->Connect() ", ERROR_SUCCESS);
				RemoveSession(seession);
				return false;
			}

			sessions.push_back(seession);
		}

		StartWorkerThread();

		std::cout << "ClinetService Started." << std::endl;
		std::cout << "Target Info : ip[ " << targetAddress.GetIpStringAddress() << " ] " << "port[ " << targetAddress.GetPort() << " ]" << std::endl;
		std::cout << "WorkerThread Count : " << _workerThreadCount << std::endl;

		return true;
	}
#elif defined(PLATFORM_LINUX)

    ServerCore::ServerCore()
    {

    }

	ServerCore::~ServerCore()
    {
		Stop();
    }

	bool ServerCore::Initialize()
	{
		_networkDispatcher = std::make_shared<LinuxEpollDispatcher>();
		if(_networkDispatcher == nullptr)
			return false;

		_acceptor = std::make_shared<Acceptor>();
		if(_acceptor == nullptr)
			return false;

		return true;
	}

	bool ServerCore::Start(NetworkAddress& listenAddress)
	{
		if(_acceptor == nullptr)
			return false;

		_isRunning.store(true);
		return true;
	}

	void ServerCore::Stop()
	{
		for(auto& workerThread : _workerThreads)
		{
			if(workerThread.joinable())
			workerThread.join();
		}
		_workerThreads.clear();

		_isRunning.store(false);
	}

	bool ServerCore::StartWorkerThread(uint32 threadCount)
	{

	}


	void ServerCore::HandleError(const char* func, int32 lineNumber, const std::string& message, int32 systemErrorCode) 
	{
		std::cerr << "[ERROR] Func: " << func << " | Line: " << lineNumber << " | Msg: " << message;
		if (systemErrorCode != 0) 
		{
			std::cerr << " | Code: " << systemErrorCode << " (0x" << std::hex << systemErrorCode << std::dec << ")";
			const char* systemErrorMessage = ::strerror(systemErrorCode);
			if (systemErrorMessage != nullptr && std::strlen(systemErrorMessage) > 0) 
				std::cerr << " (" << systemErrorMessage << ")";
		}
		std::cerr << "\n";
	}

	void ServerCore::HandleError(const char* func, int32 lineNumber, const std::string& message, ErrorCode customErrorCode) 
	{
		std::cerr << "[ERROR] Func: " << func << " | Line: " << lineNumber << " | Msg: " << message;
		if (customErrorCode != ErrorCode::Success) 
		{
			std::cerr << " | Code: " << static_cast<int32>(customErrorCode) << " (0x" << std::hex << static_cast<int32>(customErrorCode) << std::dec << ")";
			// 여기에 customErrorCode에 대한 문자열 메시지를 반환하는 함수 호출
			// 예: std::cerr << " (" << GetCustomErrorMessage(customErrorCode) << ")";
		}
		std::cerr << "\n";
	}

	std::shared_ptr<Session> ServerCore::CreateSession()
	{
		return std::shared_ptr<Session>();
	}

	void ServerCore::AddSession(std::shared_ptr<Session> session)
	{

	}

	void ServerCore::RemoveSession(std::shared_ptr<Session> session)
	{

	}

	void ServerCore::WorkerThread()
	{

	}


#endif
}