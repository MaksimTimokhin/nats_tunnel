#include "RpcGenerator.h"

bool RpcGenerator::Generate(const google::protobuf::FileDescriptor *file,
                            const std::string &parameter,
                            google::protobuf::compiler::GeneratorContext *generator_context,
                            std::string *error) const {
    auto name = file->name();
    // Cut ".pb.cc"
    name = name.substr(0, name.size() - 6);
    std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> out_h_includes(
        generator_context->OpenForInsert(name + ".pb.h", "includes"));
    google::protobuf::io::Printer pb_includes(out_h_includes.get(), '$');
    pb_includes.Print("#include <nats/rpc/Rpc.h>\n");
    pb_includes.Print("#include <memory>\n");

    std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> out_h(
        generator_context->OpenForInsert(name + ".pb.h", "namespace_scope"));
    google::protobuf::io::Printer pb_h(out_h.get(), '$');

    for (int i = 0; i < file->service_count(); ++i) {
        auto service = file->service(i);

        std::map<std::string, std::string> vars;
        vars["service_name"] = service->name();

        pb_h.Print(vars, "class $service_name$Client {\n");
        pb_h.Print("public:\n");
        pb_h.Indent();

        pb_h.Print(vars,
                   "$service_name$Client(::nats::IRpcChannel* channel) : "
                   "channel_(channel) {}\n");

        // YOUR CODE GOES HERE...
        for (int j = 0; j < service->method_count(); ++j) {
            auto method = service->method(j);
            std::map<std::string, std::string> meth_vars;
            vars["method_name"] = method->name();
            vars["output_type"] = method->output_type()->name();
            vars["input_type"] = method->input_type()->name();
            pb_h.Print(vars,
                       "std::unique_ptr<$output_type$> "
                       "$method_name$(const $input_type$& request);\n");
        }

        pb_h.Outdent();
        pb_h.Print("\nprivate:\n");

        pb_h.Indent();

        pb_h.Print("::nats::IRpcChannel* channel_ = nullptr;\n");

        pb_h.Outdent();
        pb_h.Print("};\n\n");

        pb_h.Print(vars, "class $service_name$Handler : public ::nats::IRpcService {\n");
        pb_h.Print("public:\n");
        pb_h.Indent();

        pb_h.Print(
            "void CallMethod(const google::protobuf::MethodDescriptor "
            "*method, const google::protobuf::Message &request, "
            "google::protobuf::Message *response) override;\n");
        pb_h.Print(
            "const google::protobuf::ServiceDescriptor "
            "*ServiceDescriptor() override;\n");
        for (int j = 0; j < service->method_count(); ++j) {
            auto method = service->method(j);
            std::map<std::string, std::string> meth_vars;
            meth_vars["method_name"] = method->name();
            meth_vars["output_type"] = method->output_type()->name();
            meth_vars["input_type"] = method->input_type()->name();
            pb_h.Print(meth_vars,
                       "virtual void $method_name$(const $input_type$& "
                       "request, $output_type$* response) = 0;\n");
        }
        pb_h.Outdent();
        pb_h.Print("};\n");
    }

    std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> out_cc(
        generator_context->OpenForInsert(name + ".pb.cc", "namespace_scope"));
    google::protobuf::io::Printer pb_cc(out_cc.get(), '$');

    for (int i = 0; i < file->service_count(); ++i) {
        auto service = file->service(i);
        std::map<std::string, std::string> vars;
        vars["service_name"] = service->name();
        for (int j = 0; j < service->method_count(); ++j) {
            auto method = service->method(j);
            vars["method_index"] = std::to_string(j);
            vars["method_name"] = method->name();
            vars["input_type"] = method->input_type()->name();
            vars["output_type"] = method->output_type()->name();
            pb_cc.Print(vars,
                        "std::unique_ptr<$output_type$> "
                        "$service_name$Client::$method_name$ (const "
                        "$input_type$& request) {\n");
            pb_cc.Indent();
            pb_cc.Print(vars, "auto response = std::make_unique<$output_type$>();\n");
            pb_cc.Print(vars,
                        "channel_->CallMethod($service_name$::descriptor()->"
                        "method($method_index$), request, response.get());\n");
            pb_cc.Print(vars, "return response;\n");
            pb_cc.Outdent();
            pb_cc.Print("}\n\n");
        }
    }

    for (int i = 0; i < file->service_count(); ++i) {
        auto service = file->service(i);
        std::map<std::string, std::string> vars;
        vars["service_name"] = service->name();
        pb_cc.Print(vars,
                    "void $service_name$Handler::CallMethod(const "
                    "google::protobuf::MethodDescriptor *method, const "
                    "google::protobuf::Message &request, "
                    "google::protobuf::Message *response) {\n");
        pb_cc.Indent();
        pb_cc.Print("switch (method->index()) {\n");
        pb_cc.Indent();
        for (int j = 0; j < service->method_count(); ++j) {
            auto method = service->method(j);
            std::map<std::string, std::string> meth_vars;
            meth_vars["method_index"] = std::to_string(j);
            meth_vars["method_name"] = method->name();
            meth_vars["input_type"] = method->input_type()->name();
            meth_vars["output_type"] = method->output_type()->name();
            pb_cc.Print(meth_vars, "case $method_index$:\n");
            pb_cc.Indent();
            pb_cc.Print(meth_vars,
                        "$method_name$(static_cast<const $input_type$&>(request), "
                        "static_cast<$output_type$*>(response));\nbreak;\n");
            pb_cc.Outdent();
        }
        pb_cc.Outdent();
        pb_cc.Print("}\n");
        pb_cc.Outdent();
        pb_cc.Print("}\n\n");

        pb_cc.Print(vars,
                    "const google::protobuf::ServiceDescriptor* "
                    "$service_name$Handler::ServiceDescriptor() {\n");
        pb_cc.Indent();
        pb_cc.Print(vars, "return $service_name$::descriptor();\n");
        pb_cc.Outdent();
        pb_cc.Print("}\n\n");
    }

    return true;
}