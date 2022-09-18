#pragma once
#include "WinSockWrapper.h"

#include <mutex>
#include <queue>
#include <set>
#include <atomic>

#include "APeer.h"


class AServer: APeer
{
private:
	SOCKET listenSocket = INVALID_SOCKET;

	struct ClientState: AState
	{
		AServer* server;

		std::atomic<bool> working;
		std::thread* thread;

		ClientState(SOCKET socket, AServer* server);
		~ClientState();

		static void threadFunction(ClientState* self, AServer* server);
		virtual void OnErrorStop(int code) = 0;
	};
	std::set<ClientState*> connectedClients;
	std::mutex workedUpThreadsMutex;
	std::queue<ClientState*> workedUpThreads;

	bool is_working = false;

	static void resolveAddress(const std::string& address, const std::string& port, addrinfo*& result);

	void connectNewClient(SOCKET socket);

public:
	AServer(std::string address = "", std::string port = PORT_NUMBER);

	void loop() override;

	virtual std::string messageHandler(AConnectionContext** context, std::string message) = 0;
};
