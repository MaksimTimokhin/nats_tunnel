#include "Connector.h"

const std::string Connector::kRpcPrefix = "rpc:";
const std::string Connector::kDataPrefix = "data:";

void TunnelService::Start() {
    if (!nats_client_) {
        throw std::runtime_error("Nats client is not set");
    }
    if (!nats_to_socks_redirector_) {
        nats_to_socks_redirector_ = std::make_unique<NatsToSocksRedirector>(
            nats_client_, listen_subject_, nats_to_socks_threads_);
    }
    if (!socks_to_nats_redirector_) {
        socks_to_nats_redirector_ =
            std::make_unique<SocksToNatsRedirector>(nats_client_, nats_to_socks_threads_);
    }
    nats_to_socks_redirector_->Start();
    socks_to_nats_redirector_->Start();
}

void TunnelService::Stop() {
    nats_to_socks_redirector_->Stop();
    socks_to_nats_redirector_->Stop();
}

bool TunnelService::IsRunning() const {
    return nats_to_socks_redirector_->IsRunning() || socks_to_nats_redirector_->IsRunning();
}

void TunnelService::SetNatsClient(nats::Client& nats_client) {
    nats_client_ = &nats_client;
}

void TunnelService::BuildTunnel(const TunnelRequest& request, TunnelResponse* response) {
    Poco::Net::StreamSocket conn(Poco::Net::SocketAddress{
        request.target_host(), static_cast<Poco::UInt16>(request.target_port())});
    nats_to_socks_redirector_->AddSocket(request.connection_id(), SocketDescriptor{conn});
    socks_to_nats_redirector_->AddSocket(request.connection_id(),
                                         RedirectedSocketDescriptor{conn, request.data_subject()});
    response->set_success(true);
}

void Connector::Start() {
    std::unique_lock lock(run_mutex_);
    if (is_running_.exchange(true)) {
        return;
    }
    nats_client_ = nats::Connect(nats_address_, nats_auth_);
    tunnel_service_.SetNatsClient(*nats_client_);
    server_ = nats::RpcServer(nats_client_.get(), kRpcPrefix + id_);
    server_.Register(&tunnel_service_);
    server_thread_ = std::thread([this] { server_.Serve(); });
    tunnel_service_.Start();
}

void Connector::Stop() {
    std::unique_lock lock(run_mutex_);
    if (!is_running_.exchange(false)) {
        return;
    }
    is_running_ = false;
    nats_client_->Close();
    server_thread_.join();
    tunnel_service_.Stop();
}

bool Connector::IsRunning() const {
    std::shared_lock lock(run_mutex_);
    return is_running_;
}
