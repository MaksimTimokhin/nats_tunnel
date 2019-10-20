#pragma once

#include <nats/Nats.h>
#include <tunnel/Connector.pb.h>
#include <atomic>
#include <shared_mutex>
#include <thread>
#include "AbstractTunnelComponent.h"
#include "config/Config.h"
#include "redirector/NatsToSocksRedirector.h"
#include "redirector/SocksToNatsRedirector.h"

class TunnelService : public ConnectorServiceHandler, public IExecutiveComponent {
public:
    TunnelService(int nats_to_socks_threads, int socks_to_nats_threads, std::string listen_subject)
        : nats_to_socks_threads_(nats_to_socks_threads),
          socks_to_nats_threads_(socks_to_nats_threads),
          listen_subject_(std::move(listen_subject)) {
    }
    void Start() override;
    void Stop() override;
    bool IsRunning() const override;
    void BuildTunnel(const TunnelRequest &request, TunnelResponse *response) override;
    void SetNatsClient(nats::Client &nats_client);

private:
    const int nats_to_socks_threads_, socks_to_nats_threads_;
    const std::string listen_subject_;
    nats::Client *nats_client_ = nullptr;
    std::unique_ptr<IRedirector> nats_to_socks_redirector_{nullptr},
        socks_to_nats_redirector_{nullptr};
};

class Connector : public AbstractTunnelComponent {
public:
    static const std::string kRpcPrefix;
    static const std::string kDataPrefix;
    explicit Connector(const Config &config, int socks_to_nats_threads = 1,
                       int nats_to_socks_threads = 1)
        : AbstractTunnelComponent(config),
          tunnel_service_(socks_to_nats_threads, nats_to_socks_threads, kDataPrefix + id_) {
    }

    void Start() override;
    void Stop() override;
    bool IsRunning() const override;

private:
    TunnelService tunnel_service_;
    std::unique_ptr<nats::Client> nats_client_{nullptr};
    nats::RpcServer server_;
    std::thread server_thread_;
    mutable std::shared_mutex run_mutex_;  // Start and Stop have to be atomic operations as
                                           // they create and join threads
    std::atomic_bool is_running_{false};   // atomic as threads read it while Stop writes to
};