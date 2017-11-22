#include <utility>
#include <vector>
#include <sstream>

#include "util/queue.hpp"
#include "rpc-layer/RPC.hpp"
#include "scp/node.hpp"
#include "scp/slot.hpp"
#include "rpc-layer/fakeRPC.hpp"
#include "scp/message.hpp"

using namespace DISTPROJ;

FakeRPCLayer::FakeRPCLayer() = default;

void FakeRPCLayer::AddNode(NodeID node) {
    messageQueues[node] = new Queue<std::string>();
}

MessageClient *FakeRPCLayer::GetClient(NodeID id) {
#if DEBUG
    printf("[INFO] GetClient Begin!\n");
#endif
    return new MessageClient(id, this);
}

void FakeRPCLayer::Send(std::shared_ptr<Message> msg, NodeID id, NodeID peerID) {
#if DEBUG
    printf("[INFO] FakeRPCLayer::Send() begin!\n");
#endif
    std::ostringstream ss;
    {
        cereal::JSONOutputArchive archive(ss);
#if DEBUG
        printf("[INFO] JSONOutputArchive create!\n");
#endif
        archive(CEREAL_NVP(msg));
#if DEBUG
        printf("[INFO] CEREAL_NVP\n");
#endif
    }
    messageQueues[peerID]->Add(ss.str());
#if DEBUG
    printf("[INFO] Add to messageQueues\n");
#endif
}

bool FakeRPCLayer::Receive(std::shared_ptr<Message> *msg, NodeID id) {
    // We only have 1 thread dequeing so this is chill.
    printf("%llu", id) ;
    if (messageQueues[id]->Empty()) {
        printf("IFIF\n\n");
        return false;
    } else {
        printf("ELSE\n\n");
        std::istringstream ss;
        ss.str(messageQueues[id]->Get());
        {
            cereal::JSONInputArchive archive(ss);
            archive(*msg);
        }
        return true && *msg; // implicitly checks for validity
    }
}

void FakeRPCLayer::Broadcast(std::shared_ptr<Message> msg, NodeID id, std::set<NodeID> peers) {
#if DEBUG
    printf("[INFO] FakeRPCLayer::Broadcast Begin!\n");
    printf("[INFO] NodeID: %llu\n", id);
#endif
    for (auto peer : peers) {
#if DEBUG
        printf("[INFO] peer NodeID: %llu\n", peer);
#endif
        Send(msg, id, peer);
    }
}

// MessageClient

MessageClient::MessageClient(NodeID id, RPCLayer *r) : id(id), rpc(r) {
#if DEBUG
    printf("[INFO] MessageClient Constructor Begin!\n");
#endif
}

void MessageClient::Send(std::shared_ptr<Message> msg, NodeID peerID) {
#if DEBUG
    printf("[INFO] MessageClient::Send Begin!\n");
#endif
    rpc->Send(std::move(msg), id, peerID);
}

bool MessageClient::Receive(std::shared_ptr<Message> *msg) {
#if DEBUG
    printf("[INFO] MessageClient::Receive Begin!\n");
#endif
    return rpc->Receive(msg, id);
}

void MessageClient::Broadcast(std::shared_ptr<Message> msg, std::set<NodeID> peers) {
#if DEBUG
    printf("[INFO] MessageClient::Broadcast Begin!\n");
#endif
    rpc->Broadcast(std::move(msg), id, peers);
}

