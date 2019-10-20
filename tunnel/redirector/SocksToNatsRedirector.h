#pragma once

#include <nats/Nats.h>
#include <atomic>
#include <shared_mutex>
#include <thread>
#include "ConcurrentHashMap.hpp"
#include "IRedirector.h"
#include "Poco/Net/StreamSocket.h"
#include "SocketDescriptors.h"

class SocksToNatsRedirector : public IRedirector {
private:
    const Poco::Timespan kSelectTimeout{1, 0};
    const size_t kBufferSize = 8192;

public:
    explicit SocksToNatsRedirector(nats::Client *nats_client,
                                   int threads_count = kDefaultConcurrencyLevel)
        : nats_client_(nats_client),
          threads_count_(threads_count > 0 ? threads_count : kDefaultConcurrencyLevel),
          sockets_(0, threads_count_) {
    }

    void Start() override;
    void Stop() override;
    bool IsRunning() const override;
    bool AddSocket(const std::string &socket_id,
                   const SocketDescriptor &socket_descriptor) override;
    bool RemoveSocket(const std::string &socket_id) override;

private:
    nats::Client *nats_client_;
    const int threads_count_;
    ConcurrentHashMap<std::string, RedirectedSocketDescriptor> sockets_;
    std::vector<std::thread> workers_;
    mutable std::shared_mutex run_mutex_;  // these fields are described in tunnel/Connector.h
    std::atomic_bool is_running_{false};
};