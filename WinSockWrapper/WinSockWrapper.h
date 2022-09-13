#pragma once
#include "pch.h"

#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#define PORT_NUMBER "1031"
#define MAX_COMMAND_SIZE 255
#define RETRY_NUM 5
static const char START_MESSAGE[15] = "CONNECT_CLIENT";


class WinSockWrapper
{
private:
	static bool was_initialised;

	static void init();

public:
	static void ensureInit();
	static void close();

	enum SocketResult
	{ OK, SHUTDOWN, ERR };
	static SocketResult err_send(const SOCKET* socket, const char* data, int data_len);
	static SocketResult err_recv(const SOCKET* socket, char* data, int data_len);

	static SOCKET getSocketForAddress(std::string address, std::string port);
	static std::string getIpStringFromAdress(addrinfo* info);
};

class AServer
{
public:
	struct AConnectionContext
	{
		virtual ~AConnectionContext();
	};

private:
	SOCKET listenSocket = INVALID_SOCKET;

	struct ConnectedClient
	{
		int threadId;
		std::thread* thread;

		SOCKET socket;
		AConnectionContext** context;

		ConnectedClient(int threadId, SOCKET socket, AServer* self);
		~ConnectedClient();

		static void threadFunction(int threadId, AServer* self);
		static void threadFinish(int threadId, AServer* self, ConnectedClient* client);
	};
	std::vector<ConnectedClient*> connectedClients;

	std::mutex workedUpThreadsMutex;
	std::queue<int> workedUpThreads;

	bool is_working = false;

	static void resolveAddress(const std::string& address, const std::string& port, addrinfo*& result);

	void connectNewClient(SOCKET socket);

public:
	AServer(std::string address = "", std::string port = PORT_NUMBER);
	
	void loop();

	virtual std::string messageHandler(AConnectionContext** context, std::string message) = 0;
};

class AClient
{
private:
	SOCKET socket;

	bool is_working = false;

	void loopFinish();

public:
	AClient(std::string connectionAddress = "localhost", std::string connectionPort = PORT_NUMBER);
	
	void loop();

	virtual std::string messageHandler(const std::string message) = 0;
};