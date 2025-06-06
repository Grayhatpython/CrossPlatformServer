#pragma once
#include "Types.hpp"

enum class NetworkEventType : uint8
{
	None,
	Connect,
	Disconnect,
	Accept,
	Recv,
	Send,
	Error
};

//	TEMP : linux
enum class NetworkObjectType 
{
	None,
	Acceptor,
	Session,
};

enum class ErrorCode
{
	Success = 10000,
};

enum class DispatchResult
{
	Success,
	IoError,

	Exit,
	Timeout,
	CriticalError,
};