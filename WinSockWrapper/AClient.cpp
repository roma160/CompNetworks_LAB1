#include "pch.h"
#include "AClient.h"

#include <iostream>
#include <stdexcept>

using namespace std;

// AClient -----
AClient::AClient(std::string connectionAddress, std::string connectionPort):
	APeer(), state(connectionAddress, connectionPort) {}

void AClient::loop()
{
    state.working = true;

    unique_ptr<ASockResult> buff_res;

    string receive_buff, response_buff;
    receive_buff.reserve(MAX_COMMAND_SIZE);
    response_buff.reserve(MAX_COMMAND_SIZE);
    receive_buff.resize(1);

    // Exchanging START_MESSAGEs
    if ((buff_res = communicationWrapper(this, &state, SEND, (char*)&START_MESSAGE_SIZE, 1)
        )->type != ASockResult::OK) {
        if (buff_res->type == ASockResult::ERR)
            OnErrorStop(((ErrSockResult*) buff_res.get())->code);
        return;
    }
    if ((buff_res = communicationWrapper(this, &state, SEND, (char*) START_MESSAGE, START_MESSAGE_SIZE)
        )->type != ASockResult::OK) {
        if (buff_res->type == ASockResult::ERR)
            OnErrorStop(((ErrSockResult*)buff_res.get())->code);
        return;
    }

    while(state.working)
    {
        buff_res = messagingIteration(this, &state, receive_buff, response_buff);
        if(buff_res->type != ASockResult::OK)
        {
            if (buff_res->type == ASockResult::ERR)
                OnErrorStop(((ErrSockResult*)buff_res.get())->code);
            return;
        }
    }
}

AClient::State::State(std::string connectionAddress, std::string connectionPort): AState()
{
    socket = WinSockWrapper::getSocketForAddress(connectionAddress, connectionPort);
}
