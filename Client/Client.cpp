#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

using namespace std;

#define DEFAULT_BUFLEN 512

int getSocketForAdress(const string address, const string port, SOCKET* result)
{
    int ret;

    addrinfo specs;
    ZeroMemory(&specs, sizeof(specs));
    specs.ai_family = AF_UNSPEC;
    specs.ai_socktype = SOCK_STREAM;
    specs.ai_protocol = IPPROTO_TCP;

    // RESOLVING THE HOSTNAME/ADDRESS
    addrinfo* results_list;
    if ((ret = getaddrinfo(address.c_str(), port.c_str(), &specs, &results_list)) != 0)
        return ret;

    SOCKET buff = INVALID_SOCKET;
    for(addrinfo* i = results_list; i != NULL; i = i->ai_next)
    {
        // SOCKET
        buff = socket(i->ai_family, i->ai_socktype, i->ai_protocol);
        if (buff == INVALID_SOCKET) return 1;

        // CONNECT
        if ((ret = connect(buff, i->ai_addr, (int) i->ai_addrlen)) != 0)
        {
            closesocket(buff);
            buff = INVALID_SOCKET;
	        continue;
        }
        break;
    }
    freeaddrinfo(results_list);
    if (buff == INVALID_SOCKET) return 1;

    *result = buff;
    return 0;
}

int main()
{
    // https://stackoverflow.com/questions/39931347/simple-http-get-with-winsock
    // https://docs.microsoft.com/ru-ru/windows/win32/winsock/complete-client-code

    // WinSock library init
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup failed.\n";
        return 1;
    }


    //// Attempt to connect to an address until one succeeds
    SOCKET ConnectSocket = INVALID_SOCKET;
    if(getSocketForAdress("localhost", "27015", &ConnectSocket) != 0)
    {
        WSACleanup();
        return 1;
    }

    // Send an initial buffer
    const char* sendbuf = "GET /search?q=hello HTTP/1.1\r\n"
        "Host: google.com\r\n"
        "Accept : */777*\r\n\r\n";
    if (send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0) == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
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
}