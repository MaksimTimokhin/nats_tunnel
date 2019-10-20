#pragma once

#include <Poco/Net/StreamSocket.h>
#include <component/IExecutiveComponent.h>
#include "SocketDescriptors.h"

// Redirector redirects data flow from nats subject to the set of sockets, or vice versa
class IRedirector : public IExecutiveComponent {
public:
    static const std::string kDelimiter;

    virtual bool AddSocket(const std::string &socket_id,
                           const SocketDescriptor &socket_descriptor) = 0;
    virtual bool RemoveSocket(const std::string &socket_id) = 0;

protected:
    constexpr static const int kDefaultConcurrencyLevel = 1;
};

const std::string IRedirector::kDelimiter = "|";