#include "Config.h"
#include <Poco/JSON/Parser.h>
#include <fstream>

Config::Config(const std::string &config_path) {
    std::ifstream input(config_path);
    Poco::JSON::Parser parser;
    config_ = parser.parse(input).extract<Poco::JSON::Object::Ptr>();
}

ComponentType Config::GetComponentType() const {
    auto component = config_->get("component").toString();
    if (component == "ActivePoint" || component == "AP") {
        return ComponentType::ACCESS_POINT;
    }
    if (component == "Connector") {
        return ComponentType::CONNECTOR;
    }
    throw std::runtime_error("Unknown component type");
}

Poco::Net::SocketAddress Config::GetNatsAddress() const {
    return Poco::Net::SocketAddress{config_->get("natsAddress").toString()};
}

std::string Config::GetNatsAuth() const {
    return config_->get("natsAuth").toString();
}

std::string Config::GetCommonSubject() const {
    return config_->get("commonSubject").toString();
}

std::string Config::GetId() const {
    return config_->get("id").toString();
}