//
// Created by jonas on 10/24/26.
//

#ifndef SCHEDA_INFORMATIONIS_NETBRIDGE_H
#define SCHEDA_INFORMATIONIS_NETBRIDGE_H
#include "networkManager.h"

struct LoginData
{
    std::string Username;
    std::string Password;
};

class netBridge
{
    public:
        static void sendFetchTickets(networkManager manager, const LoginData& data);
        static void sendPullTicket(networkManager manager, const LoginData& data, int id);
        static void sendVerifyUser(networkManager manager, const LoginData& data);
        static void sendCreateTicket(networkManager manager, const LoginData& data, const std::string& title, const std::string& body);
        static void sendDeleteTicket(networkManager manager, const LoginData& data, int id);
        static void sendAnswerTicket(networkManager manager, const LoginData& data, int id, const std::string& body);

};


#endif //SCHEDA_INFORMATIONIS_NETBRIDGE_H
