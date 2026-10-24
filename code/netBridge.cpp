#include "netBridge.h"

#include <jsoncons/json.hpp>

#include <cctype>
#include <stdexcept>

namespace
{
    using Json = jsoncons::json;

    void validateCredentials(const LoginData& data)
    {
        const auto hasProtocolDelimiter = [](const std::string& value) {
            return value.find('@') != std::string::npos ||
                   value.find('\n') != std::string::npos ||
                   value.find('\r') != std::string::npos;
        };

        if (data.Username.empty() || data.Password.empty())
            throw std::invalid_argument("Username and password must not be empty");
        if (hasProtocolDelimiter(data.Username) || hasProtocolDelimiter(data.Password))
            throw std::invalid_argument("Username and password cannot contain @ or newlines");
    }

    void sendCommand(networkManager& manager, const LoginData& data,
                     const std::string& command, const Json& payload)
    {
        validateCredentials(data);
        manager.sendRequest(command + "@" + data.Username + "@" + data.Password + "@" +
                            payload.to_string() + "\n");
    }

    std::string readString(const Json& object, const char* key)
    {
        if (!object.contains(key) || !object[key].is_string()) return {};
        return object[key].as<std::string>();
    }

    std::int64_t readInteger(const Json& object, const char* key)
    {
        if (!object.contains(key) || !object[key].is_number()) return 0;
        return object[key].as<std::int64_t>();
    }

    std::vector<Json> parseConcatenatedArrays(const std::string& response)
    {
        std::vector<Json> arrays;
        std::size_t position = 0;

        while (position < response.size())
        {
            while (position < response.size() &&
                   std::isspace(static_cast<unsigned char>(response[position])))
                ++position;
            if (position == response.size()) break;
            if (response[position] != '[')
                throw std::runtime_error("Ticket response must be a JSON array");

            const std::size_t start = position;
            int depth = 0;
            bool inString = false;
            bool escaped = false;
            for (; position < response.size(); ++position)
            {
                const char character = response[position];
                if (inString)
                {
                    if (escaped) escaped = false;
                    else if (character == '\\') escaped = true;
                    else if (character == '"') inString = false;
                    continue;
                }

                if (character == '"') inString = true;
                else if (character == '[' || character == '{') ++depth;
                else if (character == ']' || character == '}')
                {
                    --depth;
                    if (depth == 0)
                    {
                        ++position;
                        break;
                    }
                }
            }

            if (depth != 0 || inString)
                throw std::runtime_error("Ticket response contains incomplete JSON");
            arrays.push_back(Json::parse(response.substr(start, position - start)));
        }

        if (arrays.empty()) throw std::runtime_error("Ticket response is empty");
        return arrays;
    }

    Ticket parseTicket(const Json& value)
    {
        Ticket ticket;
        ticket.id = static_cast<int>(readInteger(value, "id"));
        ticket.title = readString(value, "title");
        ticket.status = static_cast<int>(readInteger(value, "status"));
        if (value.contains("date"))
        {
            if (value["date"].is_number())
            {
                ticket.date = value["date"].as<std::int64_t>();
                ticket.dateText = std::to_string(ticket.date);
            }
            else if (value["date"].is_string())
            {
                ticket.dateText = value["date"].as<std::string>();
                while (!ticket.dateText.empty() &&
                       (ticket.dateText.back() == '\n' || ticket.dateText.back() == '\r'))
                    ticket.dateText.pop_back();
            }
        }
        ticket.customer = readString(value, "customer");

        if (value.contains("support"))
        {
            const Json& support = value["support"];
            if (support.is_array())
            {
                for (const auto& name : support.array_range())
                    if (name.is_string()) ticket.support.push_back(name.as<std::string>());
            }
            else if (support.is_string())
            {
                const std::string name = support.as<std::string>();
                if (!name.empty() && name != "-") ticket.support.push_back(name);
            }
        }

        if (value.contains("content") && value["content"].is_array())
        {
            for (const auto& item : value["content"].array_range())
                ticket.content.push_back({readString(item, "name"), readString(item, "matter")});
        }

        // Older server snapshots kept the initial message in question/answer fields.
        if (ticket.content.empty())
        {
            const std::string question = readString(value, "question");
            const std::string answer = readString(value, "answer");
            if (!question.empty() && question != "-") ticket.content.push_back({ticket.customer, question});
            if (!answer.empty() && answer != "-") ticket.content.push_back({"Support", answer});
        }

        return ticket;
    }
}

void netBridge::sendFetchTickets(networkManager& manager, const LoginData& data)
{
    sendCommand(manager, data, "sendeTickets", Json::object());
}

void netBridge::sendCreateTicket(networkManager& manager, const LoginData& data,
                                 const std::string& title, const std::string& question)
{
    Json payload = Json::object();
    payload["title"] = title;
    payload["question"] = question;
    sendCommand(manager, data, "empfangeTickets", payload);
}

void netBridge::sendDeleteTicket(networkManager& manager, const LoginData& data, const int id)
{
    Json payload = Json::object();
    payload["id"] = id;
    sendCommand(manager, data, "loescheTicket", payload);
}

void netBridge::sendAnswerTicket(networkManager& manager, const LoginData& data,
                                 const int id, const std::string& answer)
{
    Json payload = Json::object();
    payload["id"] = id;
    payload["answer"] = answer;
    sendCommand(manager, data, "antworteTicket", payload);
}

std::vector<Ticket> netBridge::parseTickets(const std::string& response)
{
    std::vector<Ticket> tickets;
    for (const Json& array : parseConcatenatedArrays(response))
    {
        if (!array.is_array()) throw std::runtime_error("Ticket response must contain arrays");
        for (const auto& item : array.array_range()) tickets.push_back(parseTicket(item));
    }
    return tickets;
}
