#include "RpcGenerator.h"

#include <google/protobuf/compiler/plugin.h>

int main(int argc, char *argv[]) {
    RpcGenerator generator;

    return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}