// ReSharper disable CppInconsistentNaming
#pragma once
#include <atomic>
#include <memory>

#include "WinSockWrapper.h"

static constexpr char PORT_NUMBER[5] = "1037";
static constexpr int MAX_COMMAND_SIZE = 255;
static constexpr int RETRY_NUM = 5;

static constexpr char START_MESSAGE[15] = "CONNECT_CLIENT";
static constexpr int START_MESSAGE_SIZE = sizeof(START_MESSAGE);

class APeer  // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
	struct AContext  // NOLINT(cppcoreguidelines-special-member-functions)
	{ virtual ~AContext() = 0; } **context;

protected:
	std::atomic<bool> working;
	SOCKET socket;

	struct ASockResult
	{
		enum Type { OK, SHUTDOWN, ERR } type;
		ASockResult(Type type) : type(type) {}
	};
	struct ErrSockResult : ASockResult
	{
		int code;
		ErrSockResult(int code) : ASockResult(ERR), code(code) {}
	};
	enum ContactType { SEND, RECEIVE };
	std::unique_ptr<ASockResult> contact(
		ContactType type, char* data, int data_len) const;

	friend class APeerManager;
	static std::unique_ptr<ASockResult> contact(
		const SOCKET& socket, ContactType type, char* data, int data_len);

	virtual std::unique_ptr<ASockResult> loopStart(std::string& receiveBuffer, std::string& sendBuffer) = 0;
	std::unique_ptr<ASockResult> loopIterBody(std::string& receiveBuffer, std::string& sendBuffer);
	virtual std::unique_ptr<ASockResult> loopEnd(std::unique_ptr<ASockResult> reason);

public:
	APeer();
	virtual ~APeer();

	bool setSocket(SOCKET socket);
	bool setSocket(std::string connectionAddress, std::string connectionPort);

	std::unique_ptr<ASockResult> loop();

	struct HandlerResponse
	{
		std::string message;
		enum Flags
		{
			EMPTY = 0,
			MESSAGE = 1 << 0,
			SHUTDOWN = 1 << 1
		} flags;

		HandlerResponse(std::string message);
		HandlerResponse(bool is_shutdown, std::string message);
	};
	virtual HandlerResponse messageHandler(AContext** context, std::string message) = 0;
};
