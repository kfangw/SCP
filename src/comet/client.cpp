#include <utility>
#include <string>

#include "util/common.hpp"
#include "comet/client.hpp"
#include "comet/server.hpp"

using namespace DISTPROJ;

void ClientKV::Put(std::string key, std::string value) {
    PutArgs args;
    args.id = std::to_string(nrand());
    args.key = std::move(key);
    args.value = std::move(value);

    PutReply reply;

    // TODO : Do over rpc and check reply for errors.
    server->Put(args, reply);
}

std::string ClientKV::Get(std::string key) {
    GetArgs args;
    args.id = std::to_string(nrand());
    args.key = std::move(key);

    GetReply reply;

    // TODO : Do over rpc and check reply for errors.
    server->Get(args, reply);

    return reply.value;
}
