#include "Nats.h"

namespace nats {

void CheckStatus(const Status &status) {
    switch (status) {
        case NATS_OK: {
            break;
        }
        case NATS_TIMEOUT: {
            throw NatsTimeoutException();
        }
        default: {
            throw NatsException(natsStatus_GetText(status));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

Message::~Message() {
    natsMsg_Destroy(message_);
}

std::string Message::GetSubject() const {
    return natsMsg_GetSubject(message_);
}

std::string Message::GetReplySubject() const {
    return natsMsg_GetReply(message_);
}

int Message::GetSize() const {
    return natsMsg_GetDataLength(message_);
}

std::string Message::GetBody() const {
    return std::string(natsMsg_GetData(message_), natsMsg_GetDataLength(message_));
}

////////////////////////////////////////////////////////////////////////////////

void Subscription::Unsubscribe() {
    natsSubscription_Unsubscribe(subscription_);
}

std::unique_ptr<Message> Subscription::NextMessage() {
    for (;;) {
        try {
            return NextMessage(1s);
        } catch (const NatsTimeoutException &ex) {
            continue;
        }
    }
}

void Subscription::SetMessageLimit(int max_messages) {
    auto status =
        natsSubscription_SetPendingLimits(subscription_, max_messages, 1024 * max_messages);
    CheckStatus(status);
}

int Subscription::PendingCount() {
    int count;
    auto status = natsSubscription_GetPending(subscription_, &count, nullptr);
    CheckStatus(status);
    return count;
}

Subscription::~Subscription() {
    natsSubscription_Destroy(subscription_);
}

////////////////////////////////////////////////////////////////////////////////

void Client::PublishString(const std::string &subject, const std::string &string) {
    auto status =
        natsConnection_Publish(connection_, subject.c_str(),
                               reinterpret_cast<const void *>(string.c_str()), string.size());
    CheckStatus(status);
}

std::unique_ptr<Subscription> Client::Subscribe(const std::string &subject) {
    auto subscription = std::make_unique<Subscription>();
    auto status =
        natsConnection_SubscribeSync(&subscription->subscription_, connection_, subject.data());
    CheckStatus(status);
    return subscription;
}

std::unique_ptr<Subscription> Client::QueueSubscribe(const std::string &subject,
                                                     const std::string &queue_group) {
    auto subscription = std::make_unique<Subscription>();
    auto status = natsConnection_QueueSubscribeSync(&subscription->subscription_, connection_,
                                                    subject.data(), queue_group.data());
    CheckStatus(status);
    return subscription;
}

void Client::Close() {
    natsConnection_Close(connection_);
}

bool Client::IsClosed() const {
    return natsConnection_IsClosed(connection_);
}

Client::~Client() {
    natsConnection_Destroy(connection_);
}

////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<Client> Connect(const Poco::Net::SocketAddress &address, const std::string &user,
                                const std::string &password) {
    natsConnection *connection;
    std::string full_address_ = "nats://" + user + ':' + password + '@' + address.toString();
    auto status = natsConnection_ConnectTo(&connection, full_address_.data());
    CheckStatus(status);
    return std::make_unique<Client>(connection);
}

std::unique_ptr<Client> Connect(const Poco::Net::SocketAddress &address, const std::string &token) {
    natsConnection *connection;
    std::string full_address_ = "nats://" + token + '@' + address.toString();
    auto status = natsConnection_ConnectTo(&connection, full_address_.data());
    CheckStatus(status);
    return std::make_unique<Client>(connection);
}

}  // namespace nats