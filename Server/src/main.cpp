#include "Pch.hpp"

#include <atomic>
#include <thread>
#include <chrono>
#include <cassert>

#if defined(PLATFORM_WINDOWS)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

void TaskTest()
{
    std::cout << "Test3231" << std::endl;
}

#include "NetworkUtils.hpp"


class Knight
{
public:
    Knight() = default;
    ~Knight()
    {
        //std::cout << "~Knight()" << std::endl;
    }

public:
    int hp = 3;
    int mp = 0;
};



int main()
{
#if defined(PLATFORM_WINDOWS)
    ::_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    // 2. Windows 환경에서 콘솔 출력 인코딩을 UTF-8로 강제
    // 콘솔 출력 코드 페이지를 UTF-8 (65001)로 설정
    ::SetConsoleOutputCP(CP_UTF8);

    //::_CrtSetBreakAlloc(413);
#endif

    {
        std::unique_ptr<ServerCore::CoreGlobal> serverCore = std::make_unique<ServerCore::CoreGlobal>();

        /*
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < 5; i++)
        {
            ServerCore::GThreadManager->Launch([]() {
                for (int i = 0; i < 100000; i++)
                {
                    auto k1 = ServerCore::cnew<Knight>();
                    ServerCore::cdelete(k1);

                    //auto k1 = new Knight();
                    //delete k1;
                }
                });
        }
        
        ServerCore::GThreadManager->Join();

        auto end = std::chrono::high_resolution_clock::now();
        
        std::chrono::duration<double, std::milli> duration = end - start;

        std::cout << "시간 : " << duration.count() << " ms" << std::endl;

            */
        SOCKET socket = ServerCore::NetworkUtils::CreateSocket(false);

        ServerCore::NetworkUtils::Bind(socket, 8888);
        ServerCore::NetworkUtils::Listen(socket);

        SOCKET clientSocket = ::accept(socket, nullptr, nullptr);

        std::cout << "client Connected" << std::endl;

        while (true)
        {

        }
    
    }

    return 0;
} 