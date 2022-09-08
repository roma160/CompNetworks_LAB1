#pragma once

#include <string>

#define PORT_NUMBER "1031"

class WinSockWrapper
{
private:
	static bool was_initialised;

	static void init();

public:
	static void ensureInit();
	static void close();

	static SOCKET getSocketForAddress(std::string address, std::string port);
	static std::string getIpStringFromAdress(addrinfo* info);
};

