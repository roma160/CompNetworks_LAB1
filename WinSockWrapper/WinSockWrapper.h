#pragma once
#include "pch.h"
#include <string>

#define PORT_NUMBER "1031"
#define MAX_COMMAND_SIZE 255
#define RETRY_NUM 5

static const char START_MESSAGE[15] = "CONNECT_CLIENT";
constexpr int START_MESSAGE_SIZE = sizeof(START_MESSAGE);


class WinSockWrapper
{
private:
	static bool was_initialised;

	static void init();

public:
	static void ensureInit();
	static void close();

	struct SocketResult
	{
		enum ResultType
		{ OK, SHUTDOWN, ERR };

		ResultType type;
		int error_code;

		SocketResult(ResultType type);
		SocketResult(int error_code);

		static SocketResult Ok();
		static SocketResult Shutdown();
		static SocketResult Err(int code);
	};

	static SocketResult err_send(const SOCKET* socket, const char* data, int data_len);
	static SocketResult err_recv(const SOCKET* socket, char* data, int data_len);

	static SOCKET getSocketForAddress(std::string address, std::string port);
	static std::string getIpStringFromAdress(addrinfo* info);
};