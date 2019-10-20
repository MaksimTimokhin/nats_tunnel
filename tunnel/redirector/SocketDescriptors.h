#pragma once

#include <Poco/Net/StreamSocket.h>

class SocketDescriptor {
public:
    virtual ~SocketDescriptor() = default;
    explicit SocketDescriptor(const Poco::Net::StreamSocket &socket) : socket_(socket) {
    }
    Poco::Net::StreamSocket GetSocket() const;

private:
    Poco::Net::StreamSocket socket_;
};

class RedirectedSocketDescriptor : public SocketDescriptor {
public:
    RedirectedSocketDescriptor(const Poco::Net::StreamSocket &socket, std::string redirect_subject)
        : SocketDescriptor(socket), redirect_subject_(std::move(redirect_subject)) {
    }

    const std::string &GetRedirectSubject() const;

private:
    std::string redirect_subject_;
};
