#include "SocketDescriptors.h"

Poco::Net::StreamSocket SocketDescriptor::GetSocket() const {
    return socket_;
}

const std::string& RedirectedSocketDescriptor::GetRedirectSubject() const {
    return redirect_subject_;
}