#ifndef SCHEDA_INFORMATIONIS_NETWORKMANAGER_H
#define SCHEDA_INFORMATIONIS_NETWORKMANAGER_H

#ifdef SCHEDA_USE_BOOST_ASIO
#include <boost/asio.hpp>
namespace asio = boost::asio;
#else
#include <asio.hpp>
#endif

#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

/**
 * Small adapter for the ticket server's one-request-per-connection protocol.
 * Requests and their responses use the same TCP connection. Call poll()
 * regularly from the UI event loop to deliver asynchronous callbacks.
 */
class networkManager
{
public:
    using Port = std::uint16_t;
    using String = std::string;
    using ExceptionPtr = std::exception_ptr;
    using IpAddress = asio::ip::address;
    using Tcp = asio::ip::tcp;
#ifdef SCHEDA_USE_BOOST_ASIO
    using ErrorCode = boost::system::error_code;
#else
    using ErrorCode = asio::error_code;
#endif
    using ResponseHandler = std::function<void(const String&)>;
    using ErrorHandler = std::function<void(ExceptionPtr)>;

    networkManager() = default;

    networkManager(const networkManager&) = delete;
    networkManager& operator=(const networkManager&) = delete;

    ~networkManager()
    {
    }

    /** Configures the numeric IPv4 or IPv6 address used for outgoing requests. */
    void connectTo(const IpAddress& ipAddress, const Port port)
    {
        serverEndpoint_ = Tcp::endpoint(ipAddress, port);
        hasServerEndpoint_ = true;
    }

    /** Sends a newline-delimited request, optionally reading one response line. */
    void sendRequest(const String& request, const bool expectResponse = false)
    {
        if (!hasServerEndpoint_)
            throw std::logic_error("The server endpoint has not been configured");
        if (request.empty() || request.back() != '\n')
            throw std::invalid_argument("Requests must end with a newline");

        auto socket = std::make_shared<Tcp::socket>(context_);
        auto response = std::make_shared<String>();
        auto message = std::make_shared<String>(request);
        socket->async_connect(serverEndpoint_,
            [this, socket, response, message, expectResponse](const ErrorCode& error) {
                if (error)
                {
                    reportError(std::make_exception_ptr(std::runtime_error(error.message())));
                    return;
                }

                asio::async_write(*socket, asio::buffer(*message),
                    [this, socket, response, message, expectResponse](const ErrorCode& writeError, std::size_t) {
                        if (writeError)
                        {
                            reportError(std::make_exception_ptr(std::runtime_error(writeError.message())));
                            closeSocket(socket);
                            return;
                        }

                        ErrorCode ignored;
                        socket->shutdown(Tcp::socket::shutdown_send, ignored);
                        if (!expectResponse)
                        {
                            closeSocket(socket);
                            return;
                        }

                        asio::async_read_until(*socket,
                            asio::dynamic_buffer(*response, maxResponseBytes_), '\n',
                            [this, socket, response](const ErrorCode& readError, std::size_t) {
                                if (readError)
                                {
                                    reportError(std::make_exception_ptr(std::runtime_error(readError.message())));
                                }
                                else if (responseHandler_)
                                {
                                    try
                                    {
                                        responseHandler_(*response);
                                    }
                                    catch (...)
                                    {
                                        reportError(std::current_exception());
                                    }
                                }
                                closeSocket(socket);
                            });
                    });
            });
    }

    void setResponseHandler(ResponseHandler handler)
    {
        responseHandler_ = std::move(handler);
    }

    void setErrorHandler(ErrorHandler handler)
    {
        errorHandler_ = std::move(handler);
    }

    /** Processes ready Asio callbacks without blocking. */
    std::size_t poll()
    {
        context_.restart();
        return context_.poll();
    }

private:
    static constexpr std::size_t maxResponseBytes_ = 1024 * 1024;

    asio::io_context context_;
    Tcp::endpoint serverEndpoint_;
    ResponseHandler responseHandler_;
    ErrorHandler errorHandler_;
    bool hasServerEndpoint_ = false;
    static void closeSocket(const std::shared_ptr<Tcp::socket>& socket)
    {
        ErrorCode ignored;
        socket->close(ignored);
    }

    void reportError(const ExceptionPtr& error) const noexcept
    {
        try
        {
            if (errorHandler_) errorHandler_(error);
            else if (error) std::rethrow_exception(error);
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
};

#endif // SCHEDA_INFORMATIONIS_NETWORKMANAGER_H
