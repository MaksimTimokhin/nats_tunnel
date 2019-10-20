#pragma once

#include <component/IExecutiveComponent.h>
#include <nats/Nats.h>
#include "config/Config.h"

class AbstractTunnelComponent : public IExecutiveComponent {
public:
    AbstractTunnelComponent() = default;
    explicit AbstractTunnelComponent(const Config &config)
        : nats_address_(config.GetNatsAddress()),
          nats_auth_(config.GetNatsAuth()),
          common_subject_(config.GetCommonSubject()),
          id_(config.GetId()) {
    }

protected:
    const Poco::Net::SocketAddress nats_address_;
    const std::string nats_auth_;
    const std::string common_subject_;
    const std::string id_;
};