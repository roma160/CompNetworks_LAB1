#include "pch.h"
#include "AServer.h"

#include <iostream>
#include <stdexcept>

using namespace std;


void AServer::ConnectedClient::threadFunction(ConnectedClient* self, AServer* server)
{
    // Variables init

    // Exchanging START_MESSAGEs 
    

    // The main messaging loop
    while (self->working)
    {
        // Receiving client message
        if (handle_res(WinSockWrapper::err_recv(
            &self->socket, (char*)receive_buff.c_str(), 1)))
            return;
        if ((unsigned char) receive_buff[0] < 1)
            continue;

        receive_buff.resize((unsigned char) receive_buff[0]);
        if (handle_res(WinSockWrapper::err_recv(
            &self->socket, (char*)receive_buff.c_str(), (unsigned char) receive_buff[0])))
            return;

        // Doing message processing
        response_buff = server->messageHandler(self->context, receive_buff);
        response_buff = (char)response_buff.size() + response_buff;

        // Answering
        if (handle_res(WinSockWrapper::err_send(
            &self->socket, response_buff.c_str(), response_buff.size())))
            return;
    }
    self->working = false;
}

AServer::ClientState::ClientState(SOCKET socket, AServer* server) : AState()
{
    this->socket(socket);
    this->server(server);
    context(nullptr);
    thread = new std::thread(threadFunction, this, server);
}

AServer::ClientState::~ClientState()
{
    server->workedUpThreadsMutex.lock();
    server->workedUpThreads.push(this);
    server->workedUpThreadsMutex.unlock();
}

void AServer::ClientState::threadFunction(ClientState* self, AServer* server)
{
    self->working = true;

    unique_ptr<ASockResult> buff_res;

    string receive_buff, response_buff;
    receive_buff.reserve(MAX_COMMAND_SIZE);
    response_buff.reserve(MAX_COMMAND_SIZE);
    receive_buff.resize(START_MESSAGE_SIZE);

    if ((buff_res = communicationWrapper(server, self, RECEIVE, (char*)receive_buff.c_str(), 1)
        )->type != ASockResult::OK || receive_buff[0] != START_MESSAGE_SIZE)
    {
        if (buff_res->type == ASockResult::ERR)
            self->OnErrorStop(((ErrSockResult*)buff_res.get())->code);
        else if(buff_res->type == ASockResult::ERR)
        delete self;
        return;
    }

    if (handle_res(WinSockWrapper::err_recv(
        &self->socket, (char*)receive_buff.c_str(), 1)))
        return;
    if (receive_buff[0] != START_MESSAGE_SIZE)
    {
        threadFinish(self, server);
        return;
    }
    if (handle_res(WinSockWrapper::err_recv(
        &self->socket, (char*)receive_buff.c_str(), START_MESSAGE_SIZE)))
        return;
    if (!receive_buff.compare(START_MESSAGE))
    {
        threadFinish(self, server);
        return;
    }

    if (handle_res(WinSockWrapper::err_send(
        &self->socket, (const char*)&START_MESSAGE_SIZE, 1)))
        return;
    if (handle_res(WinSockWrapper::err_send(
        &self->socket, START_MESSAGE, START_MESSAGE_SIZE)))
        return;
}

void AServer::resolveAddress(const string& address, const string& port, addrinfo*& result)
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

void AServer::connectNewClient(SOCKET socket)
{
    connectedClients.insert(new ClientState(socket, this));
}

AServer::AServer(std::string address, std::string port)
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

void AServer::loop()
{
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(listenSocket);
        throw runtime_error("listenSocket failed with error: " + to_string(WSAGetLastError()));
    }

    is_working = true;

    SOCKET buffSocket = INVALID_SOCKET;
    while (is_working)
    {
        buffSocket = accept(listenSocket, nullptr, nullptr);
        if (buffSocket == INVALID_SOCKET) continue;

        connectNewClient(buffSocket);

        // Cleaning up finished connections
        ConnectedClient* buff; int i;
        workedUpThreadsMutex.lock();
        while (!workedUpThreads.empty())
        {
            buff = workedUpThreads.front();
            workedUpThreads.pop();

            buff->thread->join();
            delete buff;
            connectedClients.erase(buff);
        }
        workedUpThreadsMutex.unlock();
    }

    closesocket(listenSocket);
}