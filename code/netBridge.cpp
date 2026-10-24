//
// Created by jonas on 10/24/26.
//

#include "netBridge.h"

void netBridge::sendFetchTickets(networkManager manager, const LoginData& data)
{
    const std::string message = "fetchTickets@" + data.Username + "@" + data.Password;
    manager.sendRequest(message);
}

void netBridge::sendPullTicket(networkManager manager, const LoginData& data, const int id)
{
    const std::string message = "pullTicket@" + data.Username + "@" + data.Password + "@" + std::to_string(id);
    manager.sendRequest(message);
}

void netBridge::sendVerifyUser(networkManager manager, const LoginData& data)
{
    const std::string message = "verifyUser@" + data.Username + "@" + data.Password;
    manager.sendRequest(message);
}

void netBridge::sendCreateTicket(networkManager manager, const LoginData& data, const std::string& title, const std::string& body)
{
    const std::string message = "createTicket@" + data.Username + "@" + data.Password + "@" + title + "@" + body;
    manager.sendRequest(message);
}

void netBridge::sendDeleteTicket(networkManager manager, const LoginData& data, const int id)
{
    const std::string message = "deleteTicket@" + data.Username + "@" + data.Password + "@" + std::to_string(id);
    manager.sendRequest(message);
}

void netBridge::sendAnswerTicket(networkManager manager, const LoginData& data, const int id, const std::string& body)
{
    const std::string message = "answerTicket@" + data.Username + "@" + data.Password + "@" + std::to_string(id) + "@" + body;
    manager.sendRequest(message);
}