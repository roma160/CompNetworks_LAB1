#include <iostream>

#include "WinSockWrapper.h"

using namespace std;

class Server: public AServer
{
public:
	Server(std::string address = "", std::string port = PORT_NUMBER) : AServer(address, port){}

	struct ConnectionContext: AConnectionContext
	{
		int n = 0;

		~ConnectionContext() override
		{
			delete& n;
			cout << "Success";
		}
	};

	std::string messageHandler(AConnectionContext** _context, std::string message) override
	{
		if(*_context == nullptr)
		{
			cout << "New client!\n";
			*_context = new ConnectionContext();
		}

		cout << "Received message from a client: \n" << message<<"\n";
		ConnectionContext* context = (ConnectionContext*)*_context;
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