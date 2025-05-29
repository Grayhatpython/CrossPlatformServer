#pragma once

enum class NetworkEventType : uint8
{
	None,
	Connect,
	Disconnect,
	Accept,
	Recv,
	Send
};

enum class IocpGQCSResult
{
	Success,
	IoError,

	Exit,
	Timeout,
	CriticalError,
};