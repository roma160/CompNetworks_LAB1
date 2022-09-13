#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

#include "WinSockWrapper.h"

using namespace std;

#define DEFAULT_BUFLEN 512

//TODO: handle errors on socket opening
class Client
{
private:
    SOCKET socket;

    bool is_working = false;

public:
    Client(string connectionAddress = "localhost", string connectionPort = PORT_NUMBER)
    {
        int ret;

        WinSockWrapper::ensureInit();
        
        socket = WinSockWrapper::getSocketForAddress("localhost", PORT_NUMBER);
    }

    void clientLoop()
    {
        is_working = true;

        string sendingBuff, receiveBuff;
        receiveBuff.reserve(MAX_COMMAND_SIZE + 1);

        while(is_working)
        {
            // Sending phase
            sendingBuff = messageHandler();
			sendingBuff = (char)sendingBuff.size() + sendingBuff;
            if (send(socket, sendingBuff.c_str(), sendingBuff.size(), 0) == SOCKET_ERROR) {
                closesocket(socket);
                throw runtime_error("send failed with error: " + to_string(WSAGetLastError()));
            }

            // Receiving phase
            do {
                if (recv(socket, (char*) receiveBuff.c_str(), DEFAULT_BUFLEN, 0) > 0) {
                    printf("Bytes received:");
                    cout << recvbuf;
                }
                else {
                    printf("Connection closed\n");
                    break;
                }
            } while (true);
        }
    }

    static string messageHandler(const string message = "")
    {
        if(!message.empty())
        {
            cout << "Response from the server:\n" << message;
        }

        string ret;
        while (true) {
            cout << "Enter message for the server: ";
            getline(cin, ret);
            if (ret.size() > MAX_COMMAND_SIZE)
            {
                cout << "Size of your message is larger that " << MAX_COMMAND_SIZE << "! Correct it and send again.";
                continue;
            }
            break;
        }

        return ret;
    }
};

int main()
{
    // https://stackoverflow.com/questions/39931347/simple-http-get-with-winsock
    // https://docs.microsoft.com/ru-ru/windows/win32/winsock/complete-client-code

    WinSockWrapper::ensureInit();

    //// Attempt to connect to an address until one succeeds
    SOCKET ConnectSocket = WinSockWrapper::getSocketForAddress("localhost", PORT_NUMBER);

    // Send an initial buffer
    string buff;
    while(true)
    {
        getline(cin, buff);
	    if (send(ConnectSocket, buff.c_str(), buff.size(), 0) == SOCKET_ERROR) {
	    	printf("send failed with error: %d\n", WSAGetLastError());
	    	closesocket(ConnectSocket);
	    	WSACleanup();
	    	return 1;
	    }
    }

    // Receive until the peer closes the connection
    char recvbuf[DEFAULT_BUFLEN];
    do {
        if (recv(ConnectSocket, recvbuf, DEFAULT_BUFLEN, 0) > 0) {
            printf("Bytes received:");
            cout << recvbuf;
        }
        else {
            printf("Connection closed\n");
            break;
        }
    } while (true);

    WinSockWrapper::close();
}