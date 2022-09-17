#include "pch.h"
#include "WinSockWrapper.h"

#include <iostream>
#include <stdexcept>

using namespace std;

// Wrapper -----
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


WinSockWrapper::SocketResult::SocketResult(ResultType type)
{ this->type = type; }

WinSockWrapper::SocketResult::SocketResult(int error_code)
{
    this->error_code = error_code;
    type = ERR;
}

WinSockWrapper::SocketResult WinSockWrapper::SocketResult::Ok()
{ return SocketResult(OK); }

WinSockWrapper::SocketResult WinSockWrapper::SocketResult::Shutdown()
{ return SocketResult(SHUTDOWN); }

WinSockWrapper::SocketResult WinSockWrapper::SocketResult::Err(int code)
{ return SocketResult(code); }


WinSockWrapper::SocketResult WinSockWrapper::err_send(const SOCKET* socket, const char* data, int data_len)
{
    int result, code, i = 0;
    do
    {
	    result = send(*socket, data, data_len, 0);
        code = WSAGetLastError();

#ifdef _DEBUG
        cout<< "s" << result << "\n";
#endif

        if (result == data_len) return SocketResult::Ok();
        // https://learn.microsoft.com/uk-ua/windows/win32/winsock/windows-sockets-error-codes-2?redirectedfrom=MSDN
        if (result == 0 || code == WSAESHUTDOWN || code == WSAECONNRESET)
            return SocketResult::Shutdown();

        i++;
    } while (i < RETRY_NUM);
    return SocketResult::Err(code);
}

WinSockWrapper::SocketResult WinSockWrapper::err_recv(const SOCKET* socket, char* data, int data_len)
{
    int result, code, i = 0;
    do
    {
        result = recv(*socket, data, data_len, 0);
        code = WSAGetLastError();

#ifdef _DEBUG
        cout << "r" << result << "\n";
#endif

        if (result == data_len) return SocketResult::Ok();
        // https://learn.microsoft.com/uk-ua/windows/win32/winsock/windows-sockets-error-codes-2?redirectedfrom=MSDN
        if (result == 0 || result == WSAESHUTDOWN || code == WSAECONNRESET)
            return SocketResult::Shutdown();

        i++;
    } while (i < RETRY_NUM);
    return SocketResult::Err(code);
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

AServer::ConnectedClient::ConnectedClient(SOCKET socket, AServer* server)
{
    this->socket = socket;
    context = new AConnectionContext*(nullptr);

    thread = new std::thread(threadFunction, this, server);
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
void AServer::ConnectedClient::threadFunction(ConnectedClient* self, AServer* server)
{
    // Variables init
    self->working = true;

    string receive_buff, response_buff;
    receive_buff.reserve(MAX_COMMAND_SIZE);
    response_buff.reserve(MAX_COMMAND_SIZE);

    auto handle_res = [self, server](WinSockWrapper::SocketResult result)
    {
        if (result.type == WinSockWrapper::SocketResult::OK) return false;

        threadFinish(self, server);

        if(result.type == WinSockWrapper::SocketResult::ERR)
            throw runtime_error(to_string(result.error_code));

        return true;
    };

    // Exchanging START_MESSAGEs 
    receive_buff.resize(START_MESSAGE_SIZE);
    if (handle_res(WinSockWrapper::err_recv(
        &self->socket, (char*)receive_buff.c_str(), 1)))
        return;
    if (receive_buff[0] != START_MESSAGE_SIZE)
    {
        threadFinish(self, server);
        return;
    }
    if(handle_res(WinSockWrapper::err_recv(
        &self->socket, (char*)receive_buff.c_str(), START_MESSAGE_SIZE)))
        return;
    if (!receive_buff.compare(START_MESSAGE))
    {
        threadFinish(self, server);
        return;
    }

	if (handle_res(WinSockWrapper::err_send(
        &self->socket, (const char*) &START_MESSAGE_SIZE, 1)))
        return;
    if (handle_res(WinSockWrapper::err_send(
        &self->socket, START_MESSAGE, START_MESSAGE_SIZE)))
        return;

    // The main messaging loop
    while(self->working)
    {
        // Receiving client message
        if (handle_res(WinSockWrapper::err_recv(
            &self->socket, (char*)receive_buff.c_str(), 1)))
            return;
        if (receive_buff[0] < 1)
            continue;

        receive_buff.resize(receive_buff[0]);
        if (handle_res(WinSockWrapper::err_recv(
            &self->socket, (char*)receive_buff.c_str(), receive_buff[0])))
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

void AServer::ConnectedClient::threadFinish(ConnectedClient* self, AServer* server)
{
    // shutdown the connection since we're done
    if (shutdown(self->socket, SD_SEND) == SOCKET_ERROR) {
        throw runtime_error("shutdown failed with error: " + to_string(WSAGetLastError()));
    }
    closesocket(self->socket);

    // indicating, that we are finished
    server->workedUpThreadsMutex.lock();
    server->workedUpThreads.push(self);
    server->workedUpThreadsMutex.unlock();
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
{ connectedClients.insert(new ConnectedClient(socket, this)); }

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


void AClient::MessageHandlerResponse::init(bool is_shutdown, std::string value)
{
    this->is_shutdown = is_shutdown;
    this->value = value;
}

AClient::MessageHandlerResponse::MessageHandlerResponse(std::string value)
{ init(false, value); }

AClient::MessageHandlerResponse::MessageHandlerResponse(bool is_shutdown, std::string value)
{ init(is_shutdown, value); }


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
        if (result.type == WinSockWrapper::SocketResult::OK) return false;

        loopFinish();

        if (result.type == WinSockWrapper::SocketResult::ERR)
            throw runtime_error(to_string(result.error_code));

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
        MessageHandlerResponse response = messageHandler(receive_buff);
        if(response.is_shutdown)
        {
            loopFinish();
            return;
        }
        response_buff = move(response.value);
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
