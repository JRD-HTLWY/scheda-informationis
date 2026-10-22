//
// Created by jonas on 10/17/26.
//

#ifndef SCHEDA_INFORMATIONIS_NETWORKMANAGER_H
#define SCHEDA_INFORMATIONIS_NETWORKMANAGER_H

#include <asio.hpp>

#include <chrono>
#include <cstddef>
#include <exception>
#include <functional>
#include <iostream>
#include <istream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>

/**
 * Single-threaded TCP client for newline-delimited requests.
 *
 * Call poll() regularly from the application's event loop to process network events.
 * Requests start with a type followed by '@'; the remaining request data is passed
 * to the matching handler without conversion.
 * Pings are echoed by default and can be overridden with registerHandler().
 * If no custom error handler is set, errors are written to stderr.
 * When a peer closes the connection, the client retries the last endpoint while listening.
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
    using ErrorCode = asio::error_code;
    using RequestHandler = std::function<void(const String&)>;
    using ErrorHandler = std::function<void(ExceptionPtr)>;

    /** Creates a disconnected client with a default stderr error handler. */
    networkManager() : socket_(context_)
    {
        handlers_.emplace("ping", [this](const String& request) { handlePing(request); });
    }

    /** Cancels pending operations before destroying the client resources. */
    ~networkManager()
    {
        cancelTimer(requestTimer_);
        cancelTimer(reconnectTimer_);
        closeSocket();
    }

    /**
     * Connects synchronously to a numeric IPv4 or IPv6 address.
     * @throws asio::system_error if the connection fails.
     */
    void connectTo(const IpAddress& ipAddress, const Port port)
    {
        const Tcp::endpoint endpoint(ipAddress, port);
        socket_.connect(endpoint);
        reconnectEndpoint_ = endpoint;
        hasReconnectEndpoint_ = true;
    }

    /**
     * Writes the supplied bytes synchronously.
     *
     * The caller provides the request framing, including the newline delimiter.
     * @throws asio::system_error if the write fails.
     */
    void sendRequest(const String& request)
    {
        asio::write(socket_, asio::buffer(request));
    }

    /** Echoes a ping request back to the peer as newline-delimited data. */
    void handlePing(const String& request)
    {
        sendRequest(request + '\n');
    }

    /**
     * Registers or replaces the handler for a request type.
     * Handlers run synchronously inside poll().
     * @throws std::invalid_argument if handler is empty.
     */
    void registerHandler(const String& type, RequestHandler handler)
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
    using HandlerMap = std::map<String, RequestHandler>;

    static constexpr std::size_t maxIncomingBytes_ = 64 * 1024;
    static constexpr Duration requestTimeout_{30};

    asio::io_context context_;
    Tcp::socket socket_;
    asio::steady_timer requestTimer_{context_};
    asio::steady_timer reconnectTimer_{context_};
    asio::streambuf incoming_{maxIncomingBytes_};
    Tcp::endpoint reconnectEndpoint_;
    HandlerMap handlers_;
    ErrorHandler errorHandler_;
    bool listening_ = false;
    bool requestTimedOut_ = false;
    bool hasReconnectEndpoint_ = false;

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
        cancelSocket();
    }

    void onReadComplete(const ErrorCode& error)
    {
        cancelTimer(requestTimer_);

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
            if (isConnectionClosed(error))
            {
                recoverConnection(std::make_exception_ptr(asio::system_error(error)));
                return;
            }
            reportError(std::make_exception_ptr(asio::system_error(error)));
            return;
        }

        try
        {
            dispatchIncomingRequest();
            if (listening_) readNextRequest();
        }
        catch (const asio::system_error& exception)
        {
            if (isConnectionClosed(exception.code())) recoverConnection(std::current_exception());
            else reportError(std::current_exception());
        }
        catch (...)
        {
            reportError(std::current_exception());
        }
    }

    static bool isConnectionClosed(const ErrorCode& error) noexcept
    {
        return error == asio::error::eof ||
               error == asio::error::connection_reset ||
               error == asio::error::connection_aborted ||
               error == asio::error::broken_pipe ||
               error == asio::error::not_connected;
    }

    void recoverConnection(const ExceptionPtr& error)
    {
        if (!listening_) return;

        incoming_.consume(incoming_.size());

        cancelTimer(requestTimer_);
        closeSocket();

        notifyError(error);
        scheduleReconnect();
    }

    void scheduleReconnect()
    {
        if (!listening_ || !hasReconnectEndpoint_) return;

        reconnectTimer_.expires_after(std::chrono::seconds(1));
        reconnectTimer_.async_wait([this](const ErrorCode& error) { onReconnectTimer(error); });
    }

    void onReconnectTimer(const ErrorCode& error)
    {
        if (error == asio::error::operation_aborted || !listening_) return;
        if (error)
        {
            reportError(std::make_exception_ptr(asio::system_error(error)));
            return;
        }

        socket_.async_connect(reconnectEndpoint_,
                              [this](const ErrorCode& connectError) { onReconnectComplete(connectError); });
    }

    void onReconnectComplete(const ErrorCode& error)
    {
        if (!listening_) return;
        if (error)
        {
            closeSocket();
            notifyError(std::make_exception_ptr(asio::system_error(error)));
            scheduleReconnect();
            return;
        }

        readNextRequest();
    }

    void notifyError(const ExceptionPtr& error) const noexcept
    {
        try
        {
            if (errorHandler_) errorHandler_(error);
            else logError(error);
        }
        catch (...)
        {
            logError(std::current_exception());
        }
    }

    void reportError(const ExceptionPtr& error) noexcept
    {
        listening_ = false;
        cancelTimer(requestTimer_);
        cancelTimer(reconnectTimer_);
        cancelSocket();

        notifyError(error);
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

    static void cancelTimer(asio::steady_timer& timer)
    {
        ErrorCode cancellationError;
        timer.cancel(cancellationError);
        if (cancellationError)
            logError(std::make_exception_ptr(asio::system_error(cancellationError)));
    }

    void cancelSocket()
    {
        ErrorCode cancellationError;
        if (const ErrorCode result = socket_.cancel(cancellationError); cancellationError || result)
            logError(std::make_exception_ptr(asio::system_error(cancellationError ? cancellationError : result)));
    }

    void closeSocket()
    {
        ErrorCode closeError;
        if (const ErrorCode result = socket_.close(closeError); closeError || result)
            logError(std::make_exception_ptr(asio::system_error(closeError ? closeError : result)));
    }

    void dispatchIncomingRequest()
    {
        std::istream input(&incoming_);
        String request;
        std::getline(input, request);

        const auto separator = request.find('@');
        const String requestType = request.substr(0, separator);

        const auto handler = handlers_.find(requestType);
        if (handler == handlers_.end())
            throw std::runtime_error("No handler registered for request type: " + requestType);

        handler->second(request.substr(separator + 1));
    }

};

#endif //SCHEDA_INFORMATIONIS_NETWORKMANAGER_H