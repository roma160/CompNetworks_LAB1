#include "pch.h"
#include "APeer.h"

#include <iostream>
#include <stdexcept>

using namespace std;


DEFINE_ENUM_FLAG_OPERATORS(APeer::HandlerRespose::Flags)

APeer::AState::~AState()
{
    delete* context;
	if (shutdown(socket, SD_SEND) == SOCKET_ERROR) {
        throw runtime_error("Shutdown failed with an error: " + to_string(WSAGetLastError()));
    }
    closesocket(socket);
}

unique_ptr<APeer::ASockResult> APeer::communicationWrapper(APeer* self, AState* state,
                                                           CommunicationType type, char* data, int data_len)
{
    int result, code, i = 0;
    unique_ptr<ASockResult> ret(nullptr);
    do
    {
        if (type == RECEIVE) result = recv(state->socket, data, data_len, 0);
        else if (type == SEND) result = send(state->socket, data, data_len, 0);
        code = WSAGetLastError();

#ifdef _DEBUG
        if (type == RECEIVE) cout << "r" << result << "\n";
        else if (type == SEND) cout << "s" << result << "\n";
#endif

        if (result == data_len) return make_unique<ASockResult>(ASockResult::OK);

        // https://learn.microsoft.com/uk-ua/windows/win32/winsock/windows-sockets-error-codes-2?redirectedfrom=MSDN
        if (result == 0 || result == WSAESHUTDOWN || code == WSAECONNRESET) {
            ret = make_unique<ASockResult>(ASockResult::SHUTDOWN);
            break;
        }

        i++;
    } while (i < RETRY_NUM);

    self->messagingFinish(state);

    if (ret != nullptr) return ret;
    return make_unique<ErrSockResult>(code);
}

std::unique_ptr<APeer::ASockResult> APeer::messagingIteration(APeer* self, AState* state, std::string& receive_buff,
	std::string& response_buff)
{
    unique_ptr<ASockResult> buff_res;

    // Receiving phase
    if ((buff_res = communicationWrapper(self, state, RECEIVE, (char*)receive_buff.c_str(), 1)
        )->type != ASockResult::OK)
        return buff_res;
    if ((unsigned char) receive_buff[0] < 1)
        return make_unique<ASockResult>(ASockResult::OK);

    receive_buff.resize((unsigned char)receive_buff[0]);
    if ((buff_res = communicationWrapper(self, state, RECEIVE, (char*) receive_buff.c_str(), (unsigned char) receive_buff[0])
        )->type != ASockResult::OK)
        return buff_res;

    // Processing phase
    HandlerRespose response = self->messageHandler(*state->context, receive_buff);
    response_buff = move(response.message);
    if (response.flags & HandlerRespose::SHUTDOWN)
    {
        // In case, we need to shut down the connection
        if(response.flags & HandlerRespose::MESSAGE)
        {
            response_buff = (char) response_buff.size() + response_buff;
            if ((buff_res = communicationWrapper(self, state, SEND, (char*)response_buff.c_str(), response_buff.size())
                )->type != ASockResult::OK)
                return buff_res;
        }

        self->messagingFinish(state);
        return make_unique<ASockResult>(ASockResult::SHUTDOWN);
    }

    // Answering phase
    response_buff = (char)response_buff.size() + response_buff;
    if ((buff_res = communicationWrapper(self, state, SEND, (char*)response_buff.c_str(), response_buff.size())
        )->type != ASockResult::OK)
        return buff_res;
}

void APeer::messagingFinish(AState* state)
{ delete state; }


APeer::APeer()
{
	WinSockWrapper::ensureInit();
}

APeer::HandlerRespose::HandlerRespose(std::string message)
{
    this->message = message;
    flags = MESSAGE;
}

APeer::HandlerRespose::HandlerRespose(bool is_shutdown, std::string message)
{
    if(!message.empty())
    {
        this->message = message;
        flags |= MESSAGE;
    }
    if (is_shutdown) flags |= SHUTDOWN;
}
