#include "pch.h"
#include "WinSockWrapper.h"

#include <iostream>
#include <stdexcept>

using namespace std;

bool WinSockWrapper::was_initialised = false;

void WinSockWrapper::init()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		throw runtime_error("WSAStartup failed.");
	}
}

void WinSockWrapper::ensureInit()
{
	if(was_initialised) return;
	init();
	was_initialised = true;
}

void WinSockWrapper::close()
{
    if (!was_initialised) return;
	WSACleanup();
	was_initialised = false;
}

WinSockWrapper::SocketResult WinSockWrapper::err_send(const SOCKET* socket, const char* data, int data_len)
{
    int result, i = 0;
    do
    {
	    result = send(*socket, data, data_len, 0);
        cout<< "s" << result << "\n";
        if (result == data_len) return OK;
        if (result == WSAESHUTDOWN) return SHUTDOWN;
        i++;
    } while (i < RETRY_NUM);
    return ERR;
}

WinSockWrapper::SocketResult WinSockWrapper::err_recv(const SOCKET* socket, char* data, int data_len)
{
    int result, i = 0;
    do
    {
        result = recv(*socket, data, data_len, 0);
        cout << "r" << result << "\n";
        if (result == data_len) return OK;
        if (result == WSAESHUTDOWN) return SHUTDOWN;
        i++;
    } while (i < RETRY_NUM);
    return ERR;
}


SOCKET WinSockWrapper::getSocketForAddress(string address, string port)
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
        throw runtime_error("Error when getting list of sockets: " + to_string(ret));

    SOCKET buff = INVALID_SOCKET;
    for (addrinfo* i = results_list; i != NULL; i = i->ai_next)
    {
        // SOCKET
        buff = socket(i->ai_family, i->ai_socktype, i->ai_protocol);
        if (buff == INVALID_SOCKET)
            throw runtime_error("Error when trying to use socket: " + to_string(ret));

        // CONNECT
        if ((ret = connect(buff, i->ai_addr, (int)i->ai_addrlen)) != 0)
        {
            closesocket(buff);
            buff = INVALID_SOCKET;
            continue;
        }
        break;
    }
    freeaddrinfo(results_list);
    return buff;
}

string WinSockWrapper::getIpStringFromAdress(addrinfo* info)
{
    string ret(INET_ADDRSTRLEN, 0);
    inet_ntop(info->ai_family, &(((sockaddr_in*)info->ai_addr)->sin_addr), (PSTR) ret.c_str(), ret.size());
    return ret;
}



// AServer -----
AServer::AConnectionContext::~AConnectionContext() { }

AServer::ConnectedClient::ConnectedClient(int threadId, SOCKET socket, AServer* self)
{
    this->threadId = threadId;
    this->socket = socket;
    context = new AConnectionContext*(nullptr);

    thread = new std::thread(threadFunction, threadId, self);
}

AServer::ConnectedClient::~ConnectedClient()
{
    delete *context;
    delete thread;
}

//AServer::ConnectedClient& AServer::ConnectedClient::operator=(const ConnectedClient& to_copy)
//{
//    threadId = to_copy.threadId;
//    thread = to_copy.thread;
//    socket = to_copy.socket;
//    context = 
//}

constexpr int START_MESSAGE_SIZE = sizeof(START_MESSAGE);
void AServer::ConnectedClient::threadFunction(int threadId, AServer* self)
{
    // Variables init
    ConnectedClient* client = self->connectedClients[threadId];

    string receive_buff, response_buff;
    receive_buff.reserve(MAX_COMMAND_SIZE);
    response_buff.reserve(MAX_COMMAND_SIZE);

    auto handle_res = [threadId, self, client](WinSockWrapper::SocketResult result)
    {
        if (result == WinSockWrapper::OK) return false;

        threadFinish(threadId, self, client);

        if(result == WinSockWrapper::ERR)
			throw runtime_error(to_string(WSAGetLastError()));

        return true;
    };

    // Exchanging START_MESSAGEs 
    receive_buff.resize(START_MESSAGE_SIZE);
    if (handle_res(WinSockWrapper::err_recv(
        &client->socket, (char*)receive_buff.c_str(), 1)))
        return;
    if (receive_buff[0] != START_MESSAGE_SIZE)
    {
        threadFinish(threadId, self, client);
        return;
    }
    if(handle_res(WinSockWrapper::err_recv(
        &client->socket, (char*)receive_buff.c_str(), START_MESSAGE_SIZE)))
        return;
    if (!receive_buff.compare(START_MESSAGE))
    {
        threadFinish(threadId, self, client);
        return;
    }

	if (handle_res(WinSockWrapper::err_send(
        &client->socket, (const char*) &START_MESSAGE_SIZE, 1)))
        return;
    if (handle_res(WinSockWrapper::err_send(
        &client->socket, START_MESSAGE, START_MESSAGE_SIZE)))
        return;

    // The main messaging loop
    while(self->is_working)
    {
        // Receiving client message
        if (handle_res(WinSockWrapper::err_recv(
            &client->socket, (char*)receive_buff.c_str(), 1)))
            return;
        if (receive_buff[0] < 1)
            continue;

        receive_buff.resize(receive_buff[0]);
        if (handle_res(WinSockWrapper::err_recv(
            &client->socket, (char*)receive_buff.c_str(), receive_buff[0])))
            return;

        // Doing message processing
        response_buff = self->messageHandler(client->context, receive_buff);
        response_buff = (char)response_buff.size() + response_buff;

        // Answering
        if (handle_res(WinSockWrapper::err_send(
            &client->socket, response_buff.c_str(), response_buff.size())))
            return;
    }
}

void AServer::ConnectedClient::threadFinish(int threadId, AServer* self, ConnectedClient* client)
{
    // shutdown the connection since we're done
    if (shutdown(client->socket, SD_SEND) == SOCKET_ERROR) {
        throw runtime_error("shutdown failed with error: " + to_string(WSAGetLastError()));
    }
    closesocket(client->socket);

    // indicating, that we are finished
    self->workedUpThreadsMutex.lock();
    self->workedUpThreads.push(threadId);
    self->workedUpThreadsMutex.unlock();
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
    int threadId = 0;
    while (threadId < connectedClients.size() && 
			connectedClients[threadId]->threadId == threadId)
        threadId++;
    connectedClients.insert(
        connectedClients.begin() + threadId,
        new ConnectedClient(threadId, socket, this)
    );
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
        int buff;
        workedUpThreadsMutex.lock();
        while (!workedUpThreads.empty())
        {
            buff = workedUpThreads.front();
            workedUpThreads.pop();

            connectedClients[buff]->thread->join();
            delete connectedClients[buff];
            connectedClients.erase(connectedClients.begin() + buff);
        }
        workedUpThreadsMutex.unlock();
    }

    closesocket(listenSocket);
}



// AClient -----
AClient::AClient(std::string connectionAddress, std::string connectionPort)
{
    WinSockWrapper::ensureInit();

    socket = WinSockWrapper::getSocketForAddress("localhost", PORT_NUMBER);
}

void AClient::loop()
{
    is_working = true;

    string receive_buff, response_buff;
    receive_buff.reserve(MAX_COMMAND_SIZE);
    response_buff.reserve(MAX_COMMAND_SIZE);
    receive_buff.resize(1);

    auto handle_res = [this](WinSockWrapper::SocketResult result)
    {
        if (result == WinSockWrapper::OK) return false;

        loopFinish();

        if(result == WinSockWrapper::ERR)
            throw runtime_error(to_string(WSAGetLastError()));

        return true;
    };

    // Exchanging START_MESSAGEs
    if (handle_res(WinSockWrapper::err_send(
        &socket, (const char*) &START_MESSAGE_SIZE, 1)))
        return;
    if (handle_res(WinSockWrapper::err_send(
        &socket, START_MESSAGE, START_MESSAGE_SIZE)))
        return;

    while (is_working)
    {
        // Receiving phase
        if (handle_res(WinSockWrapper::err_recv(
            &socket, (char*)receive_buff.c_str(), 1)))
            return;
        if (receive_buff[0] < 1)
            continue;

        receive_buff.resize(receive_buff[0]);
        if (handle_res(WinSockWrapper::err_recv(
            &socket, (char*) receive_buff.c_str(), receive_buff[0])))
            return;

        // Processing phase
        response_buff = messageHandler(receive_buff);
        response_buff = (char)response_buff.size() + response_buff;

        // Answering phase
        if (handle_res(WinSockWrapper::err_send(
            &socket, response_buff.c_str(), response_buff.size())))
            return;
    }
}

void AClient::loopFinish()
{
    closesocket(socket);
}
