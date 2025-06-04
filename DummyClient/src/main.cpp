#include "Pch.hpp"  

#include "ServerCore.hpp"
#include "ServerSession.hpp"

#pragma pack(push, 1)
struct TestPacket : PacketHeader
{
    uint64 playerId;
    uint64 playerMp;
};
#pragma pack(pop)


#if defined(PLATFORM_WINDOWS)
class ParallelSessionSend
{
public:
    void ProcessParallelSend(const std::vector< std::shared_ptr<servercore::Session>>& serverSesions, int32 threadCount)
    {
        std::vector<std::thread> workerThreads;
        workerThreads.reserve(threadCount);

        const auto sessionsCount = serverSesions.size();
        const auto sessionCountPerThread = sessionsCount / threadCount;
        const auto remainSessionCount = sessionsCount % threadCount;

        size_t currentIndex = 0;

        for (int32_t i = 0; i < threadCount; i++)
        {
            size_t rangeSize = sessionCountPerThread + (i < remainSessionCount ? 1 : 0);
            if (rangeSize == 0)
                continue;

            const size_t rangeStart = currentIndex;
            const size_t rangeEnd = rangeStart + rangeSize;
            currentIndex = rangeEnd;

            servercore::GThreadManager->Launch([&serverSesions, rangeStart, rangeEnd]() {
                    for (auto j = rangeStart; j < rangeEnd; j++)
                    {
                        const auto& session = serverSesions[j];
                        if (session == nullptr)
                            continue;

                        // 1. SendBuffer 할당
                        auto segment = servercore::SendBufferArena::Allocate(sizeof(TestPacket));

                        // 2. 할당 성공 여부 명시적 확인 (assert 대체)
                        if (segment->successed == false) {
                            // 실무: 메모리 풀 고갈 상황에 대한 로그 및 통계 처리
                            std::cerr << "SendBuffer allocation failed for session: " << session->GetSessionId() << ". Skipping." << std::endl;
                            continue; // 이 세션에 대한 전송은 건너뜀
                        }

                        std::cout << "Test" << std::endl;

                        // 3. 패킷 구성
                        TestPacket* testPacket = reinterpret_cast<TestPacket*>(segment->ptr);
                        testPacket->id = 3;
                        testPacket->playerId = 3;
                        testPacket->playerMp = 6;
                        testPacket->size = sizeof(TestPacket);

                        // 4. SendContext 생성 및 전송 요청
                        auto sendContext = std::make_shared<servercore::SendContext>();
                        sendContext->sendBuffer = segment->sendBuffer;
                        sendContext->wsaBuf.buf = reinterpret_cast<CHAR*>(segment->ptr);
                        sendContext->wsaBuf.len = static_cast<ULONG>(sizeof(TestPacket));

                        session->Send(sendContext);
                    }

                    std::this_thread::sleep_for(std::chrono::seconds(1));
                });
        }
    }
};

int main()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    {
        std::function<std::shared_ptr<ServerSession>()> sessionFactory = []() {
            return servercore::MakeShared<ServerSession>();
            };

        std::shared_ptr<servercore::ClinetService> client = std::make_shared<servercore::ClinetService>(1, sessionFactory);      
        std::vector< std::shared_ptr<servercore::Session>> serverSessions;
        auto session = client->Connect(servercore::NetworkAddress("127.0.0.1", 8888), 1, serverSessions);

        ParallelSessionSend pss;
        pss.ProcessParallelSend(serverSessions, 1);

        while (true)
        {
            ;
        }
    }
    return 0;
}
#else
int main(int argc, char* argv[])
{
    SOCKET socket = INVALID_SOCKET;
    ServerCore::NetworkAddress serverAddress("127.0.0.1", 8888);
    std::string message = "Hello World";

    socket = ServerCore::NetworkUtils::CreateSocket(false);
    ::connect(socket, (struct sockaddr*)&serverAddress.GetSocketAddress(), sizeof(struct sockaddr_in));

    std::cout << "Server Connected!" << std::endl;

    bool connected = true;
    while(connected)
    {
        int32 bytesSent = ::send(socket, message.c_str(), message.length(), 0);

        if(bytesSent < 0)
        {
            connected = false;
        }
        else if(bytesSent == 0)
            connected = false;
        else if(static_cast<size_t>(bytesSent) < message.length()) 
            std::cout << "메세지 일부만 전송?" << bytesSent << "/" << message.length() << " bytes" << std::endl;
        else
            std::cout << "[" << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << "] " << " : " << message << " 패킷 전송 완료 (" << bytesSent << ")" << std::endl;

        if(connected == false)
            break;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if(socket != INVALID_SOCKET)
    {
        ServerCore::NetworkUtils::CloseSocket(socket);
        std::cout << "연결 종료!" << std::endl;
    }
 
    return 0;
}
#endif