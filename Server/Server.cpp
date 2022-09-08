#include <iostream>
#include <stdexcept>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "WinSockWrapper.h"

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

using namespace std;

class Server
{
private:
    SOCKET listenSocket = INVALID_SOCKET, clientSocket = INVALID_SOCKET;

public:
    Server(string address = "", string port = DEFAULT_PORT)
    {
        WinSockWrapper::ensureInit();

        int ret;

        // Resolve the server address and port
        addrinfo specs;
        ZeroMemory(&specs, sizeof(specs));
        specs.ai_family = AF_INET;
        specs.ai_socktype = SOCK_STREAM;
        specs.ai_protocol = IPPROTO_TCP;
        specs.ai_flags = AI_PASSIVE;

        addrinfo* result;
        // https://docs.microsoft.com/en-us/windows/win32/api/ws2tcpip/nf-ws2tcpip-getaddrinfo#:~:text=If%20the-,pNodeName,-parameter%20points%20to%20a%20string%20equal%20to%20%22localhost
        ret = getaddrinfo(
            address == "" ? NULL : address.c_str(), 
            port.c_str(), &specs, &result
        );
        if (ret != 0) throw runtime_error(
            "getaddrinfo failed with error " + to_string(ret));

        // Create a SOCKET for the server to listenSocket for clientSocket connections.
        listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (listenSocket == INVALID_SOCKET) {
            freeaddrinfo(result);
            throw runtime_error("socket failed with error: " + to_string(WSAGetLastError()));
        }

        // Setup the TCP listening socket
        ret = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
        if (ret == SOCKET_ERROR) {
            freeaddrinfo(result);
            closesocket(listenSocket);
            throw runtime_error("bind failed with error: " + to_string(ret));
        }
        freeaddrinfo(result);
        
        if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
            closesocket(listenSocket);
            throw runtime_error("listenSocket failed with error: " + to_string(WSAGetLastError()));
        }

        // Accept a clientSocket socket
        clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
        }

        // No longer need server socket
        closesocket(listenSocket);

        char recvbuf[DEFAULT_BUFLEN];
        int recvbuflen = DEFAULT_BUFLEN;
        // Receive until the peer shuts down the connection
        do {

            ret = recv(clientSocket, recvbuf, recvbuflen, 0);
            if (ret > 0) {
                printf("Bytes received: %d\n", ret);

                // Echo the buffer back to the sender
                ret = send(clientSocket, recvbuf, ret, 0);
                if (ret == SOCKET_ERROR) {
                    printf("send failed with error: %d\n", WSAGetLastError());
                    closesocket(clientSocket);
                    WSACleanup();
                }
                printf("Bytes sent: %d\n", ret);
            }
            else if (ret == 0)
                printf("Connection closing...\n");
            else {
                printf("recv failed with error: %d\n", WSAGetLastError());
                closesocket(clientSocket);
                WSACleanup();
            }

        } while (ret > 0);

        // shutdown the connection since we're done
        ret = shutdown(clientSocket, SD_SEND);
        if (ret == SOCKET_ERROR) {
            printf("shutdown failed with error: %d\n", WSAGetLastError());
            closesocket(clientSocket);
            WSACleanup();
        }

        // cleanup
        closesocket(clientSocket);
    }
};


int main()
{
    int iResult;

    // Initialize Winsock
    WinSockWrapper::ensureInit();

    Server server("");

    WinSockWrapper::close();

    return 0;
}