#include <iostream>
#include <string>

#include "AClient.h"

using namespace std;

class Client: public AClient
{
public:
    Client(std::string connectionAddress = "localhost", std::string connectionPort = PORT_NUMBER):
		AClient(connectionAddress, connectionPort) {}

    MessageHandlerResponse messageHandler(const std::string message) override
    {
        cout << "Received response from server:\n" << message << "\n";
        cout << "Enter your message (or q to quit): ";
        string ret;
        getline(cin, ret);
        if (ret == "q") return MessageHandlerResponse(true);
        return ret;
    }
};

int main()
{
    // https://stackoverflow.com/questions/39931347/simple-http-get-with-winsock
    // https://docs.microsoft.com/ru-ru/windows/win32/winsock/complete-client-code

    Client cl;
    cl.loop();

    WinSockWrapper::close();
}