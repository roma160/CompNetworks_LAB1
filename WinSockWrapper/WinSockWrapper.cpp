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

