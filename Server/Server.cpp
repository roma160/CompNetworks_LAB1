#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "WinSockWrapper.h"

using namespace std;

class Server
{
private:
    SOCKET listenSocket = INVALID_SOCKET;

    vector<int> threadIds;
    map<int, thread> clientThreads;
    mutex workedUpThreadsMutex;
    queue<int> workedUpThreads;

    bool is_working = false;


    static void resolveAdress(const string& address, const string& port, addrinfo*& result)
    {
        addrinfo specs;
        ZeroMemory(&specs, sizeof(specs));
        specs.ai_family = AF_INET;
        specs.ai_socktype = SOCK_STREAM;
        specs.ai_protocol = IPPROTO_TCP;
        specs.ai_flags = AI_PASSIVE;

        // https://docs.microsoft.com/en-us/windows/win32/api/ws2tcpip/nf-ws2tcpip-getaddrinfo#:~:text=If%20the-,pNodeName,-parameter%20points%20to%20a%20string%20equal%20to%20%22localhost
        const int ret = getaddrinfo(
            address == "" ? NULL : address.c_str(),
            port.c_str(), &specs, &result
        );
        if (ret != 0) throw runtime_error(
            "getaddrinfo failed with error " + to_string(ret));
    }

	static void threadFunction(SOCKET clientSocket, int threadId, mutex* workedUpThreadsMutex, queue<int>* workedUpThreads)
    {
        constexpr int buffer_size = 256;
        char receive_buff[buffer_size];
        int bytes_transmitted;
        do
        {
            bytes_transmitted = recv(clientSocket, receive_buff, buffer_size, 0);
            if(bytes_transmitted > 0)
            {
                cout << "Got new bytes (" << bytes_transmitted << "):\n" << receive_buff<<"\n\n";

                bytes_transmitted = send(clientSocket, receive_buff, buffer_size, 0);
                if(bytes_transmitted == SOCKET_ERROR)
                {
                    closesocket(clientSocket);
                    throw runtime_error("send failed with error: " + to_string(WSAGetLastError()));
                }
            }
        } while (bytes_transmitted > 0);

        // shutdown the connection since we're done
        if (shutdown(clientSocket, SD_SEND) == SOCKET_ERROR) {
            throw runtime_error("shutdown failed with error: " + to_string(WSAGetLastError()));
        }
        closesocket(clientSocket);

        // indicating, that we are finished
        workedUpThreadsMutex->lock();
        workedUpThreads->push(threadId);
        workedUpThreadsMutex->unlock();
    }

    int getNewThreadId()
    {
        int id = 0;
        bool ok = false;
        for (; id <= threadIds.size() && !ok; id++) {
            ok = true;
            for (int i = 0; i < threadIds.size(); i++)
                ok &= threadIds[i] != id;
	    }
        return id - 1;
    }

public:

    Server(string address = "", string port = PORT_NUMBER)
    {
        int ret;

        WinSockWrapper::ensureInit();

        // Resolve the server address and port
        addrinfo* result = nullptr;
        resolveAdress(address, port, result);

        // Create a SOCKET for the server to listen connections
        listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (listenSocket == INVALID_SOCKET) {
            freeaddrinfo(result);
            throw runtime_error("socket failed with error: " + to_string(WSAGetLastError()));
        }

        // Setup the TCP listening socket
        ret = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
        freeaddrinfo(result);
        if (ret == SOCKET_ERROR) {
            closesocket(listenSocket);
            throw runtime_error("bind failed with error: " + to_string(ret));
        }
    }

    void start()
    {
        if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
            closesocket(listenSocket);
            throw runtime_error("listenSocket failed with error: " + to_string(WSAGetLastError()));
        }
    }

    void listeningLoop()
    {
        is_working = true;

        SOCKET buffSocket = INVALID_SOCKET;
        while (is_working)
        {
            buffSocket = accept(listenSocket, nullptr, nullptr);
            if(buffSocket == INVALID_SOCKET) continue;

            // Starting new client connection
            int id = getNewThreadId();
            threadIds.push_back(id);
            clientThreads[id] = thread(threadFunction,
                buffSocket, id, &workedUpThreadsMutex, &workedUpThreads);

            // Cleaning up finished connections
            int buff;
            workedUpThreadsMutex.lock();
            while (!workedUpThreads.empty())
            {
                buff = workedUpThreads.front();
                workedUpThreads.pop();

                clientThreads[buff].join();
                threadIds.erase(find(
                    threadIds.begin(), threadIds.end(), buff));
            }
            workedUpThreadsMutex.unlock();
        }

        closesocket(listenSocket);
    }
};


int main()
{

    Server server("");
    server.start();
    server.listeningLoop();

    WinSockWrapper::close();

    return 0;
}