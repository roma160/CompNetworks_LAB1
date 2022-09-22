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
        return INVALID_SOCKET;

    SOCKET buff = INVALID_SOCKET;
    for (addrinfo* i = results_list; i != NULL; i = i->ai_next)
    {
        // SOCKET
        buff = socket(i->ai_family, i->ai_socktype, i->ai_protocol);

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

