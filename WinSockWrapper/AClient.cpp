#include "pch.h"
#include "AClient.h"

#include <iostream>
#include <stdexcept>

using namespace std;


void AClient::MessageHandlerResponse::init(bool is_shutdown, std::string value)
{
    this->is_shutdown = is_shutdown;
    this->value = value;
}

AClient::MessageHandlerResponse::MessageHandlerResponse(std::string value)
{
    init(false, value);
}

AClient::MessageHandlerResponse::MessageHandlerResponse(bool is_shutdown, std::string value)
{
    init(is_shutdown, value);
}


// AClient -----
AClient::AClient(std::string connectionAddress, std::string connectionPort)
{
    WinSockWrapper::ensureInit();

    socket = WinSockWrapper::getSocketForAddress(connectionAddress, connectionPort);
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
        &socket, (const char*)&START_MESSAGE_SIZE, 1)))
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
        if ((unsigned char) receive_buff[0] < 1)
            continue;

        receive_buff.resize((unsigned char) receive_buff[0]);
        if (handle_res(WinSockWrapper::err_recv(
            &socket, (char*)receive_buff.c_str(), (unsigned char) receive_buff[0])))
            return;

        // Processing phase
        MessageHandlerResponse response = messageHandler(receive_buff);
        if (response.is_shutdown)
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
