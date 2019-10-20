#pragma once

#include <nats/Nats.h>
#include <atomic>
#include <shared_mutex>
#include <thread>
#include "ConcurrentHashMap.hpp"
#include "IRedirector.h"
#include "SocketDescriptors.h"

class NatsToSocksRedirector : public IRedirector {
public:
    NatsToSocksRedirector(nats::Client *nats_client, std::string listen_subject,
                          int threads_count = kDefaultConcurrencyLevel)
        : nats_client_(nats_client),
          listen_subject_(std::move(listen_subject)),
          threads_count_(threads_count),
          sockets_(0, threads_count) {
    }
    void Start() override;
    void Stop() override;
    bool IsRunning() const override;
    bool AddSocket(const std::string &socket_id,
                   const SocketDescriptor &socket_descriptor) override;
    bool RemoveSocket(const std::string &socket_id) override;

private:
    nats::Client *nats_client_;
    const std::string listen_subject_;
    const int threads_count_;
    ConcurrentHashMap<std::string, SocketDescriptor> sockets_;
    std::vector<std::thread> workers_;
    mutable std::shared_mutex run_mutex_;  // these fields are described in tunnel/Connector.h
    std::atomic_bool is_running_{false};
};
