#include <iostream>
#include <thread>
#include <fstream>
#include <array>

#include "rpc-layer/RPC.hpp"
#include "rpc-layer/fakeRPC.hpp"
#include "comet/server.hpp"
#include "comet/client.hpp"

using namespace DISTPROJ;

#define N_SERVERS 3
#define N_CLIENTS 2
#define THRESHOLD 0.3

int main(int argc, char *argv[]) {
#ifdef PRINTFUNC
    printf("[FUNCTION] %s\n, ", __PRETTY_FUNCTION__);
#endif
    std::shared_ptr<FakeRPCLayer> rpc = std::make_shared<FakeRPCLayer>();

    std::set<NodeID> node_id_set;
    std::array<std::shared_ptr<ServerKV>, N_SERVERS> servers;
    for (auto i = 0; i < N_SERVERS; i++) {
        servers[i] = std::make_shared<ServerKV>(rpc, THRESHOLD);
        node_id_set.insert(servers[i]->GetNodeID());
    }
    for (auto i = 0; i < N_SERVERS; i++)
        servers[i]->AddPeers(node_id_set);


    std::array<std::shared_ptr<ClientKV>, N_CLIENTS> clients0;
    for (auto i = 0; i < N_CLIENTS; i++)
        clients0[i] = std::make_shared<ClientKV>(servers[0], "");

    std::array<std::shared_ptr<ClientKV>, N_CLIENTS> clients1;
    for (auto i = 0; i < N_CLIENTS; i++)
        clients1[i] = std::make_shared<ClientKV>(servers[1], "");

    // Test put on one client and get on another (both talk to the same server.).
    clients0[0]->Put("1", "Test");
    if (clients0[1]->Get("1") == "Test") {
        printf("Test PASSED\n");
    } else {
        printf("Test FAILED\n");
    }

    // Test get on client talking to another server.
    if (clients1[0]->Get("1") == "Test") {
        printf("Test PASSED\n");
    } else {
        printf("Test FAILED\n");
    }

    return 0;
}

