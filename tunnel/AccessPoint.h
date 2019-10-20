#pragma once

#include <nats/Nats.h>
#include "AbstractTunnelComponent.h"
#include "config/Config.h"

class AccessPoint : public AbstractTunnelComponent {
public:
    explicit AccessPoint(const Config &config) : AbstractTunnelComponent(config) {
    }

    void Start() override;
    void Stop() override;
    bool IsRunning() const override;

private:
    std::unique_ptr<nats::Client> nats_client_;
};
