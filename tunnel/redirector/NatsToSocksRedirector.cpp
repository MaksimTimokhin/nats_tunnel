#include "NatsToSocksRedirector.h"
#include <Poco/Net/SocketStream.h>

void NatsToSocksRedirector::Start() {
    std::unique_lock lock(run_mutex_);
    if (is_running_.exchange(true)) {
        return;
    }

    for (int i = 0; i < threads_count_; ++i) {
        workers_.emplace_back([this] {
            auto subscription = nats_client_->QueueSubscribe(listen_subject_, "redirectors");
            while (is_running_) {
                // TODO logging
                std::string message;
                try {
                    message = subscription->NextMessage()->GetBody();
                } catch (const nats::NatsException&) {
                    continue;
                }
                auto id_size = message.find_first_of(kDelimiter);
                if (id_size == std::string::npos) {
                    continue;
                }
                auto data = message.substr(id_size + 1);
                message.resize(id_size);
                auto& id_value = message;
                auto socket_desc = sockets_.Get(id_value);
                if (!socket_desc) {
                    continue;
                }
                Poco::Net::SocketStream(socket_desc->GetSocket()) << data << std::flush;
            }
        });
    }
}

void NatsToSocksRedirector::Stop() {
    std::unique_lock lock(run_mutex_);
    if (!is_running_.exchange(false)) {
        return;
    }

    for (auto& worker : workers_) {
        worker.join();
    }
    workers_.clear();
}

bool NatsToSocksRedirector::IsRunning() const {
    std::shared_lock lock(run_mutex_);
    return is_running_;
}

bool NatsToSocksRedirector::AddSocket(const std::string& socket_id,
                                      const SocketDescriptor& socket_descriptor) {
    return sockets_.Insert(socket_id, socket_descriptor);
}

bool NatsToSocksRedirector::RemoveSocket(const std::string& socket_id) {
    return sockets_.Erase(socket_id);
}
