#pragma once

#include <Poco/Net/SocketAddress.h>
#include <nats/nats.h>
#include <chrono>
#include <exception>
#include <memory>
#include <string>

using namespace std::chrono_literals;

namespace nats {

using Status = natsStatus;

class NatsException : public std::exception {
public:
    explicit NatsException(std::string error) : error_(std::move(error)) {
    }
    const char *what() const noexcept override {
        return error_.c_str();
    }

private:
    std::string error_;
};

class NatsTimeoutException : public NatsException {
public:
    NatsTimeoutException() : NatsException("An operation timed-out.") {
    }
};

// throws exception if result != NATS_OK
void CheckStatus(const Status &status);

class Message {
public:
    std::string GetSubject() const;
    std::string GetReplySubject() const;
    int GetSize() const;
    std::string GetBody() const;
    Message() = default;
    explicit Message(natsMsg *message) : message_(message) {
    }
    ~Message();

private:
    natsMsg *message_ = nullptr;
    friend class Subscription;
    friend class Client;
};

class Subscription {
public:
    template <typename Duration>
    std::unique_ptr<Message> NextMessage(Duration timeout);
    void SetMessageLimit(int max_messages);
    void Unsubscribe();
    std::unique_ptr<Message> NextMessage();
    int PendingCount();
    Subscription() = default;
    explicit Subscription(natsSubscription *sub) : subscription_(sub) {
    }
    ~Subscription();

private:
    natsSubscription *subscription_ = nullptr;
    friend class Client;
};

class Client {
public:
    template <typename Duration>
    void RequestReply(const std::string &subject, const std::string &request, Message *reply,
                      Duration timeout);
    void PublishString(const std::string &subject, const std::string &string);
    std::unique_ptr<Subscription> Subscribe(const std::string &subject);
    std::unique_ptr<Subscription> QueueSubscribe(const std::string &subject,
                                                 const std::string &queue_group);
    void Close();
    bool IsClosed() const;
    explicit Client(natsConnection *connection) : connection_(connection) {
    }
    Client() = default;
    ~Client();

private:
    natsConnection *connection_ = nullptr;
};

std::unique_ptr<Client> Connect(const Poco::Net::SocketAddress &address, const std::string &user,
                                const std::string &password);
std::unique_ptr<Client> Connect(const Poco::Net::SocketAddress &address, const std::string &token);

////////////////////////////////////////////////////////////////////////////////

// template methods implementation

template <typename Duration>
std::unique_ptr<Message> Subscription::NextMessage(Duration timeout) {
    auto message = std::make_unique<Message>();
    auto result = natsSubscription_NextMsg(
        &message->message_, subscription_,
        std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count());
    CheckStatus(result);
    return message;
}

template <typename Duration>
void Client::RequestReply(const std::string &subject, const std::string &request,
                          nats::Message *reply, Duration timeout) {
    auto result = natsConnection_RequestString(
        &reply->message_, connection_, subject.data(), request.data(),
        std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count());
    CheckStatus(result);
}

}  // namespace nats