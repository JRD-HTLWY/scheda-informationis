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
 * Requests go to the configured server endpoint. The current server sends ticket
 * lists back by opening a second connection to the client, so this class also
 * accepts one response connection on the configured local port.
 *
 * Call poll() regularly from the UI event loop to deliver response callbacks.
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
        ErrorCode ignored;
        responseAcceptor_.close(ignored);
        if (responseSocket_) responseSocket_->close(ignored);
    }

    /** Configures the numeric IPv4 or IPv6 address used for outgoing requests. */
    void connectTo(const IpAddress& ipAddress, const Port port)
    {
        serverEndpoint_ = Tcp::endpoint(ipAddress, port);
        hasServerEndpoint_ = true;
    }

    /** Sends one complete newline-delimited request on a fresh TCP connection. */
    void sendRequest(const String& request)
    {
        if (!hasServerEndpoint_)
            throw std::logic_error("The server endpoint has not been configured");
        if (request.empty() || request.back() != '\n')
            throw std::invalid_argument("Requests must end with a newline");

        Tcp::socket socket(context_);
        socket.connect(serverEndpoint_);
        asio::write(socket, asio::buffer(request));
        ErrorCode ignored;
        socket.shutdown(Tcp::socket::shutdown_send, ignored);
        socket.close(ignored);
    }

    /** Starts accepting server callbacks on the client's local TCP port. */
    void startListening(const Port port = 8300)
    {
        if (listening_) throw std::logic_error("The response listener is already running");
        try
        {
            responseAcceptor_.open(Tcp::v4());
            responseAcceptor_.set_option(asio::socket_base::reuse_address(true));
            responseAcceptor_.bind(Tcp::endpoint(Tcp::v4(), port));
            responseAcceptor_.listen();
            listening_ = true;
            acceptResponse();
        }
        catch (...)
        {
            ErrorCode ignored;
            responseAcceptor_.close(ignored);
            throw;
        }
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
    Tcp::acceptor responseAcceptor_{context_};
    std::shared_ptr<Tcp::socket> responseSocket_;
    String responseBuffer_;
    ResponseHandler responseHandler_;
    ErrorHandler errorHandler_;
    bool hasServerEndpoint_ = false;
    bool listening_ = false;

    void acceptResponse()
    {
        if (!listening_) return;

        responseSocket_ = std::make_shared<Tcp::socket>(context_);
        responseBuffer_.clear();
        responseAcceptor_.async_accept(*responseSocket_, [this](const ErrorCode& error) {
            if (!listening_) return;
            if (error)
            {
                reportError(std::make_exception_ptr(std::runtime_error(error.message())));
                acceptResponse();
                return;
            }

            readResponse();
        });
    }

    void readResponse()
    {
        asio::async_read(*responseSocket_, asio::dynamic_buffer(responseBuffer_, maxResponseBytes_),
                         asio::transfer_all(), [this](const ErrorCode& error, std::size_t) {
            if (error != asio::error::eof && error)
            {
                reportError(std::make_exception_ptr(std::runtime_error(error.message())));
                closeResponseSocket();
                acceptResponse();
                return;
            }

            if (responseHandler_)
            {
                try
                {
                    responseHandler_(responseBuffer_);
                }
                catch (...)
                {
                    reportError(std::current_exception());
                }
            }

            closeResponseSocket();
            acceptResponse();
        });
    }

    void closeResponseSocket()
    {
        if (!responseSocket_) return;
        ErrorCode ignored;
        responseSocket_->close(ignored);
        responseSocket_.reset();
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
