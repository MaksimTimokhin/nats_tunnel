#include "Rpc.h"
#include <nats/rpc/rpc.pb.h>
#include <memory>

namespace nats {

void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method,
                            const google::protobuf::Message &request,
                            google::protobuf::Message *response) {
    RpcRequest req;
    RpcResponse resp;
    req.set_service(method->service()->full_name());
    req.set_method(method->name());
    request.SerializeToString(req.mutable_serialized_request());

    std::string message;
    req.SerializeToString(&message);
    Message reply;
    nats_client_->RequestReply(server_subject_, message, &reply, call_timeout_);

    std::string serialized_response = reply.GetBody();
    resp.ParseFromString(serialized_response);
    auto error = resp.rpc_error();
    if (!error.message().empty()) {
        throw RpcCallError(error.message());
    }
    response->ParseFromString(resp.serialized_response());
}

void RpcServer::Serve() {
    auto listen = nats_client_->Subscribe(subject_);
    while (!nats_client_->IsClosed()) {
        try {
            auto message = listen->NextMessage();
            auto reply_subject = message->GetReplySubject();
            RpcRequest request;
            RpcResponse response;
            request.ParseFromString(message->GetBody());
            auto service = services_.at(request.service());
            auto method = service->ServiceDescriptor()->FindMethodByName(request.method());
            auto factory = google::protobuf::MessageFactory::generated_factory();
            auto request_prototype = factory->GetPrototype(method->input_type());
            auto response_prototype = factory->GetPrototype(method->output_type());
            std::unique_ptr<google::protobuf::Message> request_ptr(request_prototype->New()),
                response_ptr(response_prototype->New());
            request_ptr->ParseFromString(request.serialized_request());
            try {
                service->CallMethod(method, *request_ptr, response_ptr.get());
                response_ptr->SerializeToString(response.mutable_serialized_response());
            } catch (const std::exception &ex) {
                response.mutable_rpc_error()->set_message(ex.what());
            }
            std::string reply;
            response.SerializeToString(&reply);
            nats_client_->PublishString(reply_subject, reply);
        } catch (...) {
            continue;
        }
    }
}

}  // namespace nats
