#include "Pch.hpp"
#include "NetworkAddress.hpp"

namespace servercore
{
	NetworkAddress::NetworkAddress(const struct sockaddr_in& address)
		: _address(address)
	{

	}

	NetworkAddress::NetworkAddress(const std::string& ip, uint16 port)
	{
		std::memset(&_address, 0, sizeof(_address));
		_address.sin_family = AF_INET;
		_address.sin_addr = IpStringAddressToSocketAddress(ip.c_str());
		_address.sin_port = ::htons(port);
	}

	std::string NetworkAddress::GetIpStringAddress()
	{
		char buffer[INET_ADDRSTRLEN] = {};

		assert(::inet_ntop(AF_INET, &_address.sin_addr, buffer, sizeof(buffer)));
		return std::string(buffer);
	}

	struct in_addr NetworkAddress::IpStringAddressToSocketAddress(const char* ip)
	{
		struct in_addr address;
		std::memset(&address, 0, sizeof(address));

		assert(::inet_pton(AF_INET, ip, &address) == 1);
		return address;
	}
}