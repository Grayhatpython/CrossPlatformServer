#pragma once	

namespace ServerCore
{
	class NetworkAddress
	{
	public:
		NetworkAddress() = default;
		NetworkAddress(const struct sockaddr_in& address);
		NetworkAddress(const std::string& ip, uint16 port);

	public:
		struct sockaddr_in&		GetSocketAddress() { return _address; }
		std::string				GetIpStringAddress();
		uint16					GetPort() const { return ::ntohs(_address.sin_port); }

	public:
		static struct in_addr	IpStringAddressToSocketAddress(const char* ip);

	private:
		struct sockaddr_in	_address = {};
	};
}