#pragma once

#include <google/protobuf/descriptor.h>
#include <nats/Nats.h>

namespace nats {

class IRpcChannel {
public:
    virtual ~IRpcChannel() = default;

    virtual void CallMethod(const google::protobuf::MethodDescriptor *method,
                            const google::protobuf::Message &request,
                            google::protobuf::Message *response) = 0;
};

class IRpcService : public IRpcChannel {
public:
    virtual const google::protobuf::ServiceDescriptor *ServiceDescriptor() = 0;
};

class RpcCallError : public std::exception {
public:
    explicit RpcCallError(std::string error) : error_(std::move(error)) {
    }

    const char *what() const noexcept override {
        return error_.c_str();
    }

private:
    std::string error_;
};

class RpcChannel : public IRpcChannel {
public:
    template <typename Duration>
    RpcChannel(Client *nats_client, std::string server_subject, Duration call_timeout)
        : nats_client_(nats_client),
          server_subject_(std::move(server_subject)),
          call_timeout_(std::chrono::duration_cast<std::chrono::milliseconds>(call_timeout)) {
    }

    void CallMethod(const google::protobuf::MethodDescriptor *method,
                    const google::protobuf::Message &request,
                    google::protobuf::Message *response) override;

private:
    Client *nats_client_;
    std::string server_subject_;
    std::chrono::milliseconds call_timeout_;
};

class RpcServer {
public:
    RpcServer() = default;
    RpcServer(Client *nats_client, std::string subject)
        : nats_client_(nats_client), subject_(std::move(subject)) {
    }

    void Register(IRpcService *service) {
        services_[service->ServiceDescriptor()->full_name()] = service;
    }

    // that's blocking operation,
    void Serve();

private:
    Client *nats_client_ = nullptr;
    std::string subject_;
    std::unordered_map<std::string, IRpcService *> services_;
};

}  // namespace nats