#include "Pch.hpp"
#include "ServerCore.hpp"
#include "Acceptor.hpp"
#include "Session.hpp"

namespace servercore
{
	ServerCore::ServerCore(uint16 port, int32 workerThreadCount)
		: _listenNetworkAddress("0.0.0.0", port)
	{
		NetworkUtils::Initialize(); 

		_core = new CoreGlobal();

		_workerThreadCount = workerThreadCount > 0 ? workerThreadCount : std::thread::hardware_concurrency();

		_iocpCore = std::make_shared<IocpCore>();
		_acceptor = std::make_shared<Acceptor>(_iocpCore, this);
	}

	ServerCore::~ServerCore()
	{
		Stop();
		NetworkUtils::Clear();
	}

	bool ServerCore::Start()
	{
		if (_isRunning.exchange(true) == true)
			return false;

		//	TODO : 50
		if (_acceptor->Start(_listenNetworkAddress, 50) == false)
		{
			HandleError(__FUNCTION__, __LINE__, "ConnectEx Failed : ", ERROR_SUCCESS);
			_isRunning.store(false);
			return false;
		}

		_iocpWorkerThreads.reserve(_workerThreadCount);
		for (auto i = 0; i < _workerThreadCount; i++)
			_iocpWorkerThreads.emplace_back(&ServerCore::IocpWorkerThread, this);

		std::cout << "ServerCore Started." << std::endl;
		std::cout << "Listening Info : ip[ " << _listenNetworkAddress.GetIpStringAddress() << " ] " << "port[ " << _listenNetworkAddress.GetPort() << " ]" <<  std::endl;
		std::cout << "WorkerThread Count : " << _workerThreadCount << std::endl;

		return true;
	}

	void ServerCore::Stop()
	{
		if (_isRunning.load() == false)
			return;

		std::cout << "ServerCore stopping..." << std::endl;

		{
			//	acceptor clear
			if (_acceptor)
				_acceptor->Close();	//	TODO

			//	acceptor clear wait 
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}

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
				_iocpCore->PostExitSignal();

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
			//	core clear
			_acceptor.reset();
			_iocpCore.reset();

			if (_core)
			{
				delete _core;
				_core = nullptr;
			}
		}

		std::cout << "ServerCore stopped..." << std::endl;
	}
	
	//	TODO
	void ServerCore::OnSessionConnected(std::shared_ptr<Session> session)
	{

	}
	void ServerCore::OnSessionDisconnected(std::shared_ptr<Session> session)
	{
	}
	void ServerCore::OnSessionRecv(std::shared_ptr<Session> session, const BYTE* data, int32 length)
	{
		//	TEST
		std::cout << data << " : " << length << std::endl;
	}
	void ServerCore::OnClientSessionConnected(std::shared_ptr<Session> session)
	{
	}
	void ServerCore::OnClientSessionDisconnected(std::shared_ptr<Session> session, int32 errorCode)
	{
	}

	std::shared_ptr<Session> ServerCore::CreateClientSessionAndConnect(NetworkAddress& targetAddress)
	{
		if (_isRunning.exchange(true) == true)
		{
			HandleError(__FUNCTION__, __LINE__, "Server not running", ERROR_SUCCESS);
			return nullptr;
		}

		_iocpWorkerThreads.reserve(_workerThreadCount);
		for (auto i = 0; i < _workerThreadCount; i++)
			_iocpWorkerThreads.emplace_back(&ServerCore::IocpWorkerThread, this);

		std::cout << "ServerCore Started." << std::endl;
		std::cout << "Target Info : ip[ " << targetAddress.GetIpStringAddress() << " ] " << "port[ " << targetAddress.GetPort() << " ]" << std::endl;
		std::cout << "WorkerThread Count : " << _workerThreadCount << std::endl;

		auto clientSession = std::make_shared<Session>(_iocpCore, this);

		AddSession(clientSession);

		if (clientSession->Connect(targetAddress) == false) {
			HandleError(__FUNCTION__, __LINE__, "clientSession->Connect() ", ERROR_SUCCESS);
			RemoveSession(clientSession);
			return nullptr;
		}

		return clientSession;
	}

	void ServerCore::HandleError(const char* func, int32 lineNumber, const std::string& message, int32 errorCode)
	{
		std::cerr << "[ERROR] Func: " << func << " | Line: " << lineNumber << " | Msg: " << message << " | Code: " << errorCode << " (0x" << std::hex << errorCode << std::dec << ")\n";
	}

	void ServerCore::IocpWorkerThread()
	{
		GThreadManager->InitializeThreadLocal();

		while (_isRunning.load() == true)
		{
			IocpGQCSResult result = _iocpCore->Dispatch(100);

			//	CriticalError는 ... 아직 처리 고민
			if (result == IocpGQCSResult::Exit || result == IocpGQCSResult::CriticalError)
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
}