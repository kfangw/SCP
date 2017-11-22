#ifndef STELLAR_CLIENT_KV_H
#define STELLAR_CLIENT_KV_H

#include <string>
#include <random>

#include "scp/node.hpp"
#include "misc.hpp"

namespace DISTPROJ {

    class LocalNode;

    class RPCLayer;

    class ServerKV;

    class ClientKV {

        std::shared_ptr<ServerKV> server;
        std::string name;

    public:

        ClientKV(const std::shared_ptr<ServerKV> &svr, std::string nm)
                : server(std::move(svr)), name(std::move(nm)) {};

        void Put(std::string key, std::string value);

        std::string Get(std::string key);

    };

}

#endif
