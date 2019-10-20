#pragma once

#include <string>

#include <google/protobuf/io/zero_copy_stream.h>

#include <google/protobuf/io/printer.h>

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/descriptor.h>

class RpcGenerator : public google::protobuf::compiler::CodeGenerator {
public:
    virtual bool Generate(const google::protobuf::FileDescriptor *file,
                          const std::string &parameter,
                          google::protobuf::compiler::GeneratorContext *generator_context,
                          std::string *error) const override;
};