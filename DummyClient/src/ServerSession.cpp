#include "Pch.hpp"
#include "ServerSession.hpp"

ServerSession::ServerSession()
{
	std::cout << "ServerSession" << std::endl;
}

ServerSession::~ServerSession()
{
	std::cout << "~ServerSession" << std::endl;
}

void ServerSession::OnConnected()
{

}

void ServerSession::OnDisconnected()
{

}

void ServerSession::OnRecv(BYTE* buffer, int32 numOfBytes)
{
	std::cout << buffer << std::endl;
}

void ServerSession::OnSend()
{
	std::cout << "OnSend" << std::endl;
}
