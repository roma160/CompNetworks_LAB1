#pragma once
#include "WinSockWrapper.h"
#include <memory>

#define PORT_NUMBER "1031"
#define MAX_COMMAND_SIZE 255
#define RETRY_NUM 5

static const char START_MESSAGE[15] = "CONNECT_CLIENT";
constexpr int START_MESSAGE_SIZE = sizeof(START_MESSAGE);

class APeer
{
public:
	struct AContext
	{ virtual ~AContext() = 0; };

protected:
	struct AState
	{
		SOCKET socket;
		AContext** context;

		virtual ~AState();
	};

	struct ASockResult
	{
		enum Type { OK, SHUTDOWN, ERR } type;
		ASockResult(Type type) : type(type){}
	};
	struct ErrSockResult: ASockResult
	{
		int code;
		ErrSockResult(int code) : ASockResult(ERR), code(code) {}
	};
	enum CommunicationType { SEND, RECEIVE };
	static std::unique_ptr<ASockResult> communicationWrapper(
		APeer* self, AState* state, CommunicationType type, char* data, int data_len);

	static std::unique_ptr<ASockResult> messagingIteration(APeer* self, AState* state, std::string& receive_buff, std::string& response_buff);
	virtual void messagingFinish(AState* state);

public:
	APeer();

	virtual void loop() = 0;

	struct HandlerRespose
	{
		std::string message;

		enum Flags
		{
			MESSAGE = 1<<0,
			SHUTDOWN = 1<<1
		} flags;

		HandlerRespose(std::string message);
		HandlerRespose(bool is_shutdown, std::string message);
	};
	virtual HandlerRespose messageHandler(AContext* state, std::string message);
};

