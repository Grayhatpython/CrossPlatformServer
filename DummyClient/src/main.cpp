#include "Pch.hpp"  

#if defined(PLATFORM_WINDOWS)
void HandleError(const char* cause)
{
    int32 errorCode = ::WSAGetLastError();
    std::cout << cause << " ErrorCode : " << errorCode << std::endl;
}

int main()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    WSADATA wsaData;
    if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return 0;

    SOCKET clientSocket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET)
        return 0;

    u_long on = 1;
    if (::ioctlsocket(clientSocket, FIONBIO, &on) == INVALID_SOCKET)
        return 0;

    SOCKADDR_IN serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    ::inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
    serverAddr.sin_port = htons(8888);

    while (true)
    {
        if (::connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
        {
            if (::WSAGetLastError() == WSAEWOULDBLOCK)
                continue;
            if (::WSAGetLastError() == WSAEISCONN)
                break;

            break;
        }
    }

    std::cout << "Connected to Server!" << std::endl;

    char sendBuffer[100] = "Hello World";
    WSAEVENT wsaEvent = ::WSACreateEvent();
    WSAOVERLAPPED overlapped = {};
    overlapped.hEvent = wsaEvent;

    while (true)
    {
        WSABUF wsaBuf;
        wsaBuf.buf = sendBuffer;
        wsaBuf.len = 100;

        DWORD sendLen = 0;
        DWORD flags = 0;
        if (::WSASend(clientSocket, &wsaBuf, 1, &sendLen, flags, &overlapped, nullptr) == SOCKET_ERROR)
        {
            if (::WSAGetLastError() == WSA_IO_PENDING)
            {
                ::WSAWaitForMultipleEvents(1, &wsaEvent, TRUE, WSA_INFINITE, FALSE);
                ::WSAGetOverlappedResult(clientSocket, &overlapped, &sendLen, FALSE, &flags);
            }
            else
                break;
        }

        std::cout << "Send Data! Len = " << sizeof(sendBuffer) << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    ::closesocket(clientSocket);

    ::WSACleanup();

    return 0;
}
#elif
int main(int argc, char* argv[])
{
    return 0;
}
#endif