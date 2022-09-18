#pragma once
#include "APeer.h"
#include "WinSockWrapper.h"

class AClient : public APeer
{
protected:
	std::unique_ptr<ASockResult> loopStart(std::string& receiveBuffer, std::string& sendBuffer) override;

public:
	AClient(std::string connectionAddress = "localhost", std::string connectionPort = PORT_NUMBER);
};