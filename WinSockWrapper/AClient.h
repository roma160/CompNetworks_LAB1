#pragma once
#include "APeer.h"
#include "WinSockWrapper.h"

class AClient : protected APeer
{
private:
	struct State : AState
	{
		bool working = false;

		State(std::string connectionAddress, std::string connectionPort);
	} state;

protected:
	virtual void OnErrorStop(int code) = 0;

public:

	AClient(std::string connectionAddress = "localhost", std::string connectionPort = PORT_NUMBER);

	void loop() override;
};