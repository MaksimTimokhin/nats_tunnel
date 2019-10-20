#include "AccessPoint.h"

void AccessPoint::Start() {
    nats_client_ = nats::Connect(nats_address_, nats_auth_);
}