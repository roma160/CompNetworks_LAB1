#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

#include "WinSockWrapper.h"

using namespace std;

#define DEFAULT_BUFLEN 512

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