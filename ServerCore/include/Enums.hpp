#pragma once

enum class NetworkEventType : uint8
{
	None,
	Connect,
	Accept,
	Recv,
	Send
};