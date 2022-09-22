#include "pch.h"
#include "AClient.h"

#include <iostream>
#include <stdexcept>

using namespace std;


std::unique_ptr<APeer::ASockResult> AClient::loopStart(std::string& receiveBuffer, std::string& sendBuffer)
{
    unique_ptr<ASockResult> buffRes;

    // Exchanging START_MESSAGEs
    if ((buffRes = contact(SEND, (char*) &START_MESSAGE_SIZE, 1)
        )->type != ASockResult::OK)
        return buffRes;
    if ((buffRes = contact(SEND, (char*) START_MESSAGE, START_MESSAGE_SIZE)
        )->type != ASockResult::OK)
        return buffRes;

    return buffRes;
}

AClient::AClient(std::string connectionAddress, std::string connectionPort): APeer()
{ setSocket(move(connectionAddress), move(connectionPort)); }
