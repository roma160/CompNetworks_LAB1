#pragma once
#include "pch.h"

#include <mutex>
#include <queue>
#include <string>
#include <set>
#include <atomic>

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

class AClient
{
private:
	SOCKET socket;

	bool is_working = false;

	void loopFinish();

public:
	class MessageHandlerResponse
	{
	private:
		void init(bool is_shutdown, std::string value);

	public:
		bool is_shutdown;
		std::string value;

		MessageHandlerResponse(std::string value = "");
		MessageHandlerResponse(bool is_shutdown, std::string value = "");
	};

	AClient(std::string connectionAddress = "localhost", std::string connectionPort = PORT_NUMBER);
	
	void loop();

	virtual MessageHandlerResponse messageHandler(const std::string message) = 0;
};