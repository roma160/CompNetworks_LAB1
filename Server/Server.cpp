#include <iostream>

#include "AServer.h"

using namespace std;

class Server: public AServer
{
public:
	Server(std::string address = "", std::string port = PORT_NUMBER) : AServer(move(address), move(port)){}

	struct Context: APeer::AContext
	{
		int n = 0;

		virtual ~Context()
		{
			cout << "Removed finished client!\n";
		}
	};

	APeer::HandlerResponse messageHandler(APeer::AContext** _context, std::string message) override
	{
		if(*_context == nullptr)
		{
			cout << "New client!\n";
			*_context = new Context();
		}

		cout << "Received message from a client: \n" << message<<"\n";
		Context* context = (Context*)*_context;
		context->n += 1;
		return "This is " + to_string(context->n) + "th message.";
	}
};

int main()
{
    Server server;
	server.loop();

    WinSockWrapper::close();

    return 0;
}