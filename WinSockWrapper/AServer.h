#pragma once
#include "WinSockWrapper.h"

#include <mutex>
#include <queue>
#include <set>
#include <atomic>


class AServer
{
public:
	struct AConnectionContext
	{ virtual ~AConnectionContext(); };

private:
	SOCKET listenSocket = INVALID_SOCKET;

	struct ConnectedClient
	{
		std::atomic<bool> working;
		std::thread* thread;

		SOCKET socket;
		AConnectionContext** context;

		ConnectedClient(SOCKET socket, AServer* server);
		~ConnectedClient();

		static void threadFunction(ConnectedClient* self, AServer* server);
		static void threadFinish(ConnectedClient* self, AServer* server);
	};
	std::set<ConnectedClient*> connectedClients;

	std::mutex workedUpThreadsMutex;
	std::queue<ConnectedClient*> workedUpThreads;

	bool is_working = false;

	static void resolveAddress(const std::string& address, const std::string& port, addrinfo*& result);

	void connectNewClient(SOCKET socket);

public:
	AServer(std::string address = "", std::string port = PORT_NUMBER);

	void loop();

	virtual std::string messageHandler(AConnectionContext** context, std::string message) = 0;
};