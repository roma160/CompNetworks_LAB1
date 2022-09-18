#include <iostream>
#include <string>

#include "AClient.h"

using namespace std;

class Client: public AClient
{
public:
	struct Context: AContext
	{
		int i = 0;

        virtual ~Context() = default;
	};

    Client(std::string connectionAddress = "localhost", std::string connectionPort = PORT_NUMBER):
		AClient(move(connectionAddress), move(connectionPort)) {}

    HandlerResponse messageHandler(AContext** context, std::string message) override
    {
        if (*context == nullptr) *context = new Context();
        Context* c = (Context*) *context;
        if(c->i++ < 1)
        {
            return HandlerResponse("Test");
        }
        else
        {
            return HandlerResponse(true, string(""));
        }
    }
};

int main()
{
    // https://stackoverflow.com/questions/39931347/simple-http-get-with-winsock
    // https://docs.microsoft.com/ru-ru/windows/win32/winsock/complete-client-code
	for(int i = 0; i < 10000; i++){
		Client cl;
		cl.loop();
	}

    WinSockWrapper::close();
    cin.get();
}