#pragma once

#pragma pack(push, 1)
struct TestPacket : PacketHeader
{
	uint64 playerId;
	uint64 playerMp;
};
#pragma pack(pop)


class ClientSession : public servercore::Session
{
public:
	ClientSession();
	~ClientSession() override;

public:
	virtual		void OnConnected() override;
	virtual		void OnDisconnected() override;
	virtual		void OnRecv(BYTE* buffer, int32 numOfBytes) override;
	virtual		void OnSend() override;

private:

};
