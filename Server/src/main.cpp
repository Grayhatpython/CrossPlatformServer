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


#include "ServerCore.hpp"
#include "ClientSession.hpp"



int main()
{
#if defined(PLATFORM_WINDOWS)
    ::_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    // 2. Windows 환경에서 콘솔 출력 인코딩을 UTF-8로 강제
    // 콘솔 출력 코드 페이지를 UTF-8 (65001)로 설정
    ::SetConsoleOutputCP(CP_UTF8);

    //::_CrtSetBreakAlloc(270);
#endif
    
    {
        std::function<std::shared_ptr<ClientSession>()> sessionFactory = []() {
            return servercore::MakeShared<ClientSession>();
            };

        std::shared_ptr<servercore::ServerService> server = std::make_shared<servercore::ServerService>(1, sessionFactory);
        server->Start(8888);

        char input;

        while (true)
        {
            std::cin >> input;

            if (input == 'q' || input == 'Q')
                break;
        }

        server->Stop();

        /*
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < 5; i++)
        {
            ServerCore::GThreadManager->Launch([]() {
                for (int i = 0; i < 100000; i++)
                {
                    //auto k1 = ServerCore::cnew<Knight>();
                    //ServerCore::cdelete(k1);

                    auto k1 = new Knight();
                    delete k1;
                }
                },"TestThread", false);
        }
        
        ServerCore::GThreadManager->Join();

        auto end = std::chrono::high_resolution_clock::now();
        
        std::chrono::duration<double, std::milli> duration = end - start;

        std::cout << "시간 : " << duration.count() << " ms" << std::endl;
        */
           
    }

    return 0;
} 








