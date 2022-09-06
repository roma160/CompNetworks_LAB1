#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

using namespace std;

#define DEFAULT_BUFLEN 512

int main()
{
    cout << "g";

    // https://stackoverflow.com/questions/39931347/simple-http-get-with-winsock

    // WinSock library init
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup failed.\n";
        return 1;
    }

    //--------------------------------
    // Setup the hints address info structure
    // which is passed to the getaddrinfo() function
    struct addrinfo* result = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo("google.com", "80", &hints, &result) != 0) {
        cout << "getaddrinfo failed.\n";
        WSACleanup();
        return 1;
    }

    //// Attempt to connect to an address until one succeeds
    SOCKET ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    for (struct addrinfo* ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            cout << "socket failed.\n";
            WSACleanup();
            return 1;
        }

        // Connect to server.
        if (connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }
    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        cout << "Unable to connect to server!\n";
        WSACleanup();
        return 1;
    }

    // Send an initial buffer
    const char* sendbuf = "GET /search?q=hello HTTP/1.1\r\n"
        "Host: google.com\r\n"
        "Accept : */*\r\n\r\n";
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