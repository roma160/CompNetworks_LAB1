#include "pch.h"
#include "AServer.h"

using namespace std;


std::unique_ptr<APeer::ASockResult> AServer::Peer::loopEnd(std::unique_ptr<ASockResult> reason)
{
    reason = APeer::loopEnd(move(reason));

    // Signalling work end
    manager->finishedPeersMutex.lock();
    manager->finishedPeers.push(this);
    manager->finishedPeersMutex.unlock();

    return reason;
}

std::unique_ptr<APeer::ASockResult> AServer::Peer::loopStart(std::string& receiveBuffer, std::string& sendBuffer)
{
    unique_ptr<ASockResult> buffRes;

    // Receiving start message from the client
    if ((buffRes = contact(RECEIVE, (char*)receiveBuffer.c_str(), 1)
        )->type != ASockResult::OK || (unsigned char)receiveBuffer[0] != START_MESSAGE_SIZE)
        return buffRes;
    if ((buffRes = contact(RECEIVE, (char*)receiveBuffer.c_str(), START_MESSAGE_SIZE)
        )->type != ASockResult::OK || !receiveBuffer.compare(START_MESSAGE))
        return buffRes;

    // Responding him with the confirmation message
    if ((buffRes = contact(SEND, (char*)&START_MESSAGE_SIZE, 1)
        )->type != ASockResult::OK)
        return buffRes;
    if ((buffRes = contact(SEND, (char*)START_MESSAGE, START_MESSAGE_SIZE)
        )->type != ASockResult::OK)
        return buffRes;

    return buffRes;
}

AServer::Peer::Peer(AServer* manager, SOCKET socket) :
    APeer(), manager(manager)
{
    // https://stackoverflow.com/questions/10998780/stdthread-calling-method-of-class
    thread = new std::thread(&Peer::loop, this);
    setSocket(socket);
}

AServer::Peer::~Peer()
{
    thread->join();
    delete thread;
}

APeer::HandlerResponse AServer::Peer::messageHandler(AContext** context, std::string message)
{
    return manager->messageHandler(context, move(message));
}

void AServer::resolveAddress(const std::string& address, const std::string& port, addrinfo*& result)
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
}

AServer::AServer(std::string address, std::string port) : working(false)
{
    int ret;

    WinSockWrapper::ensureInit();

    // Resolve the server address and port
    addrinfo* result = nullptr;
    resolveAddress(address, port, result);

    // Create a SOCKET for the server to listen connections
    listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listenSocket == INVALID_SOCKET) {
        freeaddrinfo(result);
        throw runtime_error("listenSocket failed with error: " + to_string(WSAGetLastError()));
    }

    // Setup the TCP listening listenSocket
    ret = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
    freeaddrinfo(result);

    if (ret == SOCKET_ERROR) {
        closesocket(listenSocket);
        throw runtime_error("bind failed with error: " + to_string(ret));
    }
}

void AServer::loop()
{
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(listenSocket);
        throw runtime_error("listenSocket failed with error: " + to_string(WSAGetLastError()));
    }

    working = true;

    while (working)
    {
        const SOCKET buffSocket = accept(listenSocket, nullptr, nullptr);
        if (buffSocket == INVALID_SOCKET) continue;

        peers.insert(new Peer(this, buffSocket));

        // Cleaning up finished connections
        Peer* buff;
        finishedPeersMutex.lock();
        while (!finishedPeers.empty())
        {
            buff = finishedPeers.front();
            finishedPeers.pop();

            delete buff;
            peers.erase(buff);
        }
        finishedPeersMutex.unlock();
    }

    closesocket(listenSocket);
}
