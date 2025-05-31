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

int main()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    {
        std::function<std::shared_ptr<ServerSession>()> sessionFactory = []() {
            return servercore::MakeShared<ServerSession>();
            };

        std::shared_ptr<servercore::ClinetService> client = std::make_shared<servercore::ClinetService>(2, sessionFactory);      
        std::vector< std::shared_ptr<servercore::Session>> serverSessions;
        auto session = client->Connect(servercore::NetworkAddress("127.0.0.1", 8888), 1, serverSessions);

        while (true)
        {
            //  패킷생성해서 Send 테스트..
            //  좀 사용하기 난잡하고 가변 길이 처리도 안됨.. 
            //  수정필요
            auto segment = servercore::SendBufferArena::Allocate(sizeof(TestPacket));
            if (segment->successed == false)
                assert(false);  //  ???

            TestPacket* testPacket = reinterpret_cast<TestPacket*>(segment->ptr);
            testPacket->id = 3;
            testPacket->playerId = 3;
            testPacket->playerMp = 6;
            testPacket->size = sizeof(TestPacket);

            auto sendContext = std::make_shared<servercore::SendContext>();
            sendContext->sendBuffer = segment->sendBuffer;
            sendContext->wsaBuf.buf = reinterpret_cast<CHAR*>(segment->ptr);
            sendContext->wsaBuf.len = static_cast<ULONG>(sizeof(TestPacket));


            for (auto& serverSession : serverSessions)
                serverSession->Send(sendContext);

            std::this_thread::sleep_for(std::chrono::seconds(1));
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