//
// Created by jonas on 10/17/26.
//

#ifndef SCHEDA_INFORMATIONIS_NETWORKMANAGER_H
#define SCHEDA_INFORMATIONIS_NETWORKMANAGER_H

#include <jsoncons/json.hpp>
#include <asio.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <istream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

/**
 * Single-threaded TCP client for newline-delimited JSON requests.
 *
 * Call poll() regularly from the application's event loop to process network
 * events. Incoming JSON may contain a numeric "type" field; requests without
 * one are treated as pings. Pings are echoed by default and can be overridden
 * with registerHandler(). If no custom error handler is set, errors are written
 * to stderr.
 */
class networkManager
{
public:
    using RequestId = std::uint32_t;
    using Port = std::uint16_t;
    using String = std::string;
    using ExceptionPtr = std::exception_ptr;
    using IpAddress = asio::ip::address;
    using Tcp = asio::ip::tcp;
    using Json = jsoncons::json;
    using ErrorCode = asio::error_code;
    using RequestHandler = std::function<void(const Json&)>;
    using ErrorHandler = std::function<void(ExceptionPtr)>;

    enum class RequestType : RequestId
    {
        Ping = 0,
        Fetch = 1,
        Pull = 2
    };

    /** Creates a disconnected client with a default stderr error handler. */
    networkManager() : socket_(context_)
    {
        handlers_[RequestType::Ping] = [this](const Json& request) { handlePing(request); };
    }

    networkManager(const networkManager&) = delete;
    networkManager& operator=(const networkManager&) = delete;

    /** Cancels pending operations before destroying the client resources. */
    ~networkManager()
    {
        ErrorCode ignored;
        requestTimer_.cancel(ignored);
        if (const ErrorCode closeResult = socket_.close(ignored); ignored || closeResult)
        {
            logError(std::make_exception_ptr(asio::system_error(ignored ? ignored : closeResult)));
        }
    }

    /**
     * Connects synchronously to a numeric IPv4 or IPv6 address.
     * @throws asio::system_error if the connection fails.
     */
    void connectTo(const IpAddress& ipAddress, const Port port)
    {
        socket_.connect(Tcp::endpoint(ipAddress, port));
    }

    /**
     * Writes the supplied bytes synchronously.
     *
     * The caller provides the JSON serialization and framing, including the
     * newline delimiter.
     * @throws asio::system_error if the write fails.
     */
    void sendRequest(const String& request)
    {
        asio::write(socket_, asio::buffer(request));
    }

    /** Echoes a ping request back to the peer as newline-delimited JSON. */
    void handlePing(const Json& request)
    {
        std::ostringstream response;
        response << request << '\n';
        sendRequest(response.str());
    }

    /**
     * Registers or replaces the handler for a request type.
     * Handlers run synchronously inside poll().
     * @throws std::invalid_argument if handler is empty.
     */
    void registerHandler(const RequestType type, RequestHandler handler)
    {
        if (!handler) throw std::invalid_argument("Request handler must not be empty");
        handlers_[type] = std::move(handler);
    }

    /**
     * Replaces the error callback. Passing an empty callback restores stderr
     * logging as the fallback.
     */
    void setErrorHandler(ErrorHandler handler)
    {
        errorHandler_ = std::move(handler);
    }

    /**
     * Starts asynchronous reading. Call poll() regularly to process events.
     * Requests time out after 30 seconds; lines larger than 64 KiB are errors.
     * @throws std::logic_error if listening has already started.
     */
    void startListening()
    {
        if (listening_) throw std::logic_error("The listener has already been started");
        listening_ = true;
        readNextRequest();
    }

    /**
     * Processes ready network events without blocking.
     * Call regularly from the application's event loop.
     * @return Number of event handlers invoked.
     */
    std::size_t poll()
    {
        context_.restart();
        return context_.poll();
    }

private:
    using Duration = std::chrono::seconds;
    using HandlerMap = std::map<RequestType, RequestHandler>;

    static constexpr std::size_t maxIncomingBytes_ = 64 * 1024;
    static constexpr Duration requestTimeout_{30};

    asio::io_context context_;
    Tcp::socket socket_;
    asio::steady_timer requestTimer_{context_};
    asio::streambuf incoming_{maxIncomingBytes_};
    HandlerMap handlers_;
    ErrorHandler errorHandler_;
    bool listening_ = false;
    bool requestTimedOut_ = false;

    void readNextRequest()
    {
        requestTimedOut_ = false;
        requestTimer_.expires_after(requestTimeout_);
        requestTimer_.async_wait([this](const ErrorCode& error) { onTimeout(error); });
        asio::async_read_until(socket_, incoming_, '\n',
                               [this](const ErrorCode& error, std::size_t) { onReadComplete(error); });
    }

    void onTimeout(const ErrorCode& error)
    {
        if (error == asio::error::operation_aborted || !listening_) return;
        if (error)
        {
            reportError(std::make_exception_ptr(asio::system_error(error)));
            return;
        }

        requestTimedOut_ = true;
        ErrorCode cancelError;
        if (const ErrorCode cancelResult = socket_.cancel(cancelError); cancelError || cancelResult)
        {
            reportError(std::make_exception_ptr(asio::system_error(cancelError ? cancelError : cancelResult)));
        }
    }

    void onReadComplete(const ErrorCode& error)
    {
        ErrorCode cancelError;
        requestTimer_.cancel(cancelError);
        if (cancelError)
        {
            reportError(std::make_exception_ptr(asio::system_error(cancelError)));
            return;
        }

        const bool timedOut = requestTimedOut_;
        requestTimedOut_ = false;

        if (!listening_) return;
        if (error == asio::error::operation_aborted && timedOut)
        {
            incoming_.consume(incoming_.size());
            readNextRequest();
            return;
        }
        if (error)
        {
            reportError(std::make_exception_ptr(asio::system_error(error)));
            return;
        }

        try
        {
            dispatchIncomingRequest();
            if (listening_) readNextRequest();
        }
        catch (...)
        {
            reportError(std::current_exception());
        }
    }

    void reportError(const ExceptionPtr& error) noexcept
    {
        listening_ = false;
        ErrorCode ignored;
        requestTimer_.cancel(ignored);
        if (const ErrorCode cancelResult = socket_.cancel(ignored); ignored || cancelResult)
        {
            logError(std::make_exception_ptr(asio::system_error(ignored ? ignored : cancelResult)));
        }

        try
        {
            if (errorHandler_)
            {
                errorHandler_(error);
            }
            else
            {
                logError(error);
            }
        }
        catch (...)
        {
            logError(std::current_exception());
        }
    }

    static void logError(const ExceptionPtr& error) noexcept
    {
        try
        {
            if (error) std::rethrow_exception(error);
        }
        catch (const std::exception& exception)
        {
            std::cerr << "networkManager: " << exception.what() << '\n';
        }
        catch (...)
        {
            std::cerr << "networkManager: unknown error\n";
        }
    }

    void dispatchIncomingRequest()
    {
        const auto request = parseIncomingRequest();
        const auto typeId = request.contains("type")
                                ? request.at("type").as<RequestId>()
                                : static_cast<RequestId>(RequestType::Ping);
        const auto type = parseRequestType(typeId);
        const auto handler = handlers_.find(type);
        if (handler == handlers_.end())
        {
            throw std::runtime_error("No handler registered for request type: " + std::to_string(typeId));
        }

        handler->second(request);
    }

    Json parseIncomingRequest()
    {
        std::istream input(&incoming_);
        String payload;
        std::getline(input, payload);
        if (!payload.empty() && payload.back() == '\r') payload.pop_back();
        return Json::parse(payload);
    }

    static RequestType parseRequestType(const RequestId typeId)
    {
        if (typeId == static_cast<RequestId>(RequestType::Ping)) return RequestType::Ping;
        if (typeId == static_cast<RequestId>(RequestType::Fetch)) return RequestType::Fetch;
        if (typeId == static_cast<RequestId>(RequestType::Pull)) return RequestType::Pull;
        throw std::invalid_argument("Unknown request type: " + std::to_string(typeId));
    }
};

#endif //SCHEDA_INFORMATIONIS_NETWORKMANAGER_H
