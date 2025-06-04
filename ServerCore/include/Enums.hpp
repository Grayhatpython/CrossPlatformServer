#pragma once
#include "Types.hpp"

enum class NetworkEventType : uint8
{
	None,
	Connect,
	Disconnect,
	Accept,
	Recv,
	Send
};

enum class DispatchResult
{
	Success,
	IoError,

	Exit,
	Timeout,
	CriticalError,
};