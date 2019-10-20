#include "SocksToNatsRedirector.h"
#include <Poco/FIFOBuffer.h>
#include <Poco/Net/SocketStream.h>
#include <algorithm>
#include <unordered_set>

void SocksToNatsRedirector::Start() {
    std::unique_lock lock(run_mutex_);
    if (is_running_.exchange(true)) {
        return;
    }

    for (int thread_id = 0; thread_id < threads_count_; ++thread_id) {
        workers_.emplace_back([this, thread_id] {
            Poco::FIFOBuffer buffer(kBufferSize);

            while (is_running_) {
                auto observed_items = sockets_.GetStripeItems(thread_id);

                Poco::Net::Socket::SocketList ready_to_read, empty_write_list, empty_except_list;
                ready_to_read.reserve(observed_items.size());
                for (const auto& [_, socket_descriptor] : observed_items) {
                    ready_to_read.push_back(socket_descriptor.GetSocket());
                }

                Poco::Net::Socket::select(ready_to_read, empty_write_list, empty_except_list,
                                          kSelectTimeout);

                // using hash set of pointers to remove not ready sockets in linear time
                std::unordered_set<Poco::Net::SocketImpl*> ready_pointers;
                ready_pointers.reserve(ready_to_read.size());
                for (const auto& socket : ready_to_read) {
                    ready_pointers.insert(socket.impl());
                }

                observed_items.erase(
                    std::remove_if(
                        observed_items.begin(), observed_items.end(),
                        [&ready_pointers](
                            const std::pair<std::string, RedirectedSocketDescriptor>& item) {
                            return !ready_pointers.count(item.second.GetSocket().impl());
                        }),
                    observed_items.end());

                for (const auto& [conn_id, socket_descriptor] : observed_items) {
                    int bytes_received = socket_descriptor.GetSocket().receiveBytes(buffer);
                    std::string data(bytes_received, 0);
                    buffer.read(data.data(), bytes_received);
                    nats_client_->PublishString(socket_descriptor.GetRedirectSubject(),
                                                conn_id + kDelimiter += data);
                }
            }
        });
    }
}

void SocksToNatsRedirector::Stop() {
    std::unique_lock lock(run_mutex_);
    if (!is_running_.exchange(false)) {
        return;
    }

    for (auto& worker : workers_) {
        worker.join();
    }
}

bool SocksToNatsRedirector::IsRunning() const {
    std::shared_lock lock(run_mutex_);
    return is_running_;
}

bool SocksToNatsRedirector::AddSocket(const std::string& socket_id,
                                      const SocketDescriptor& socket_descriptor) {
    try {
        return sockets_.Insert(socket_id,
                               dynamic_cast<const RedirectedSocketDescriptor&>(socket_descriptor));
    } catch (const std::bad_cast& exception) {
        std::cerr << exception.what();
        return false;
    }
}

bool SocksToNatsRedirector::RemoveSocket(const std::string& socket_id) {
    return sockets_.Erase(socket_id);
}
