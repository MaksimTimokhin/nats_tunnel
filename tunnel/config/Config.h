#pragma once

#include "tunnel/ComponentType.h"
#include <Poco/JSON/Object.h>
#include <Poco/Net/SocketAddress.h>

class Config {
  public:
    explicit Config(const std::string &config_path);
    ComponentType GetComponentType() const;
    Poco::Net::SocketAddress GetNatsAddress() const;
    std::string GetNatsAuth() const;
    std::string GetCommonSubject() const;
    std::string GetId() const;

  private:
    Poco::JSON::Object::Ptr config_;
};
