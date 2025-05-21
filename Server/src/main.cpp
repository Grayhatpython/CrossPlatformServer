#include "Pch.hpp"

#include <atomic>
#include <thread>
#include <chrono>
#include <cassert>

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

void TaskTest()
{
    std::cout << "Test3231" << std::endl;
}

int main()
{
#ifdef _WIN32
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    // 2. Windows 환경에서 콘솔 출력 인코딩을 UTF-8로 강제
    // 콘솔 출력 코드 페이지를 UTF-8 (65001)로 설정
    ::SetConsoleOutputCP(CP_UTF8);
#endif

    {
        ServerCore::GThreadManager->InitializeThreadPool(1);

        ServerCore::GThreadManager->PushTask(TaskTest, "test");

        ServerCore::GThreadManager->Launch([]() {
            std::cout << ServerCore::LThreadId << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            }, "dbthread");

        while (true)
            std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
} 