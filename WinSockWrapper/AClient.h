#pragma once
#include "WinSockWrapper.h"

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