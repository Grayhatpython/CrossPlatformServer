#pragma once

#pragma once

class ServerSession : public servercore::Session
{
public:
	ServerSession();
	~ServerSession() override;

public:
	virtual		void OnConnected() override;
	virtual		void OnDisconnected() override;
	virtual		void OnRecv(BYTE* buffer, int32 numOfBytes) override;
	virtual		void OnSend() override;

private:

};