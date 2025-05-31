#include "Pch.hpp"
#include "ClientSession.hpp"

ClientSession::ClientSession()
{
	std::cout << "ClientSession" << std::endl;
}

ClientSession::~ClientSession()
{
	std::cout << "~ClientSession" << std::endl;
}

void ClientSession::OnConnected()
{

}

void ClientSession::OnDisconnected()
{

}

void ClientSession::OnRecv(BYTE* buffer, int32 numOfBytes)
{
	TestPacket* testPacket = reinterpret_cast<TestPacket*>(buffer);

	std::cout << "id : " << testPacket->id << " size : " << testPacket->size << " playerId : " << testPacket->playerId << " playerMp : " << testPacket->playerMp << std::endl;
}

void ClientSession::OnSend()
{

}
