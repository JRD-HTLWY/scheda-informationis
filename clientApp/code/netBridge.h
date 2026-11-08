#ifndef SCHEDA_INFORMATIONIS_NETBRIDGE_H
#define SCHEDA_INFORMATIONIS_NETBRIDGE_H

#include "networkManager.h"

#include <cstdint>
#include <string>
#include <vector>

struct LoginData
{
    std::string Username;
    std::string Password;
};

struct TicketMessage
{
    std::string name;
    std::string matter;
};

struct Ticket
{
    int id = 0;
    std::string title;
    int status = 0;
    std::int64_t date = 0;
    std::string dateText;
    std::string customer;
    std::vector<std::string> support;
    std::vector<TicketMessage> content;
};

/** Encodes the ticket-server commands and decodes ticket-list responses. */
class netBridge
{
public:
    static void sendFetchTickets(networkManager& manager, const LoginData& data);
    static void sendCreateTicket(networkManager& manager, const LoginData& data,
                                 const std::string& title, const std::string& question);
    static void sendDeleteTicket(networkManager& manager, const LoginData& data, int id);
    static void sendAnswerTicket(networkManager& manager, const LoginData& data,
                                 int id, const std::string& answer);

    static std::vector<Ticket> parseTickets(const std::string& response);
};

#endif // SCHEDA_INFORMATIONIS_NETBRIDGE_H
