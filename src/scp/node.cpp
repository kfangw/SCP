
#include <iostream>
#include <set>
#include "scp/message.hpp"
#include "rpc-layer/fakeRPC.hpp"
#include "scp/slot.hpp"
#include "scp/node.hpp"

using namespace DISTPROJ;

Node::Node(NodeID _id, const std::shared_ptr<RPCLayer> _rpc)
        : id(_id), rpc(_rpc), t(nullptr) {
    rpc->AddNode(id);
}

Node::Node(NodeID _id, const std::shared_ptr<RPCLayer> _rpc, Quorum _quorumSet)
        : id(_id), rpc(_rpc), quorumSet(std::move(_quorumSet)), t(nullptr) {
    rpc->AddNode(id);
}

NodeID Node::GetNodeID() {
    return id;
}

Quorum Node::GetQuorumSet() {
    return quorumSet;
}

#ifdef USED
void Node::PrintQuorumSet() {
    printf("Printing quorum set for node %llu \n", id);
    printf("Threshold: %i ", quorumSet.threshold);
    printf("Quorum members : \n");
    std::set<NodeID>::iterator iter;
    for (iter = quorumSet.members.begin(); iter != quorumSet.members.end(); ++iter) {
        std::cout << (*iter) << "\n";
    }
}
#endif

LocalNode::LocalNode(NodeID _id, std::shared_ptr<RPCLayer> _rpc)
        : Node(_id, _rpc) {
    mc = _rpc->GetClient(_id);
};

LocalNode::LocalNode(NodeID _id, std::shared_ptr<RPCLayer> _rpc, Quorum _quorumSet)
        : Node(_id, _rpc, std::move(_quorumSet)) {
    mc = _rpc->GetClient(_id);
};

void LocalNode::Tick() {
    std::shared_ptr<Message> m;
    while (true) {
        std::lock_guard<std::mutex> lock(mtx);
        if (ReceiveMessage(&m)) {
            ProcessMessage(m);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void LocalNode::Start() {
    if (t == nullptr) {
        t = new std::thread(&LocalNode::Tick, this);
    }
}

void LocalNode::AddKnownNode(NodeID v) {
    knownNodes.insert(v);
}

void LocalNode::UpdateQurorum(Quorum _quorumSet) {
    quorumSet = std::move(_quorumSet);
}

void LocalNode::AddNodeToQuorum(NodeID v) {
    quorumSet.members.insert(v);
}

void LocalNode::RemoveNodeFromQuorum(NodeID v) {
    quorumSet.members.erase(v);
}

unsigned long LocalNode::QuorumSize() {
    return quorumSet.members.size();

}

void LocalNode::SetThreshold(unsigned long t) {
    if (t > QuorumSize()) {
        quorumSet.threshold = QuorumSize();
    } else {
        quorumSet.threshold = t;
    }
}

unsigned long LocalNode::GetThreshold() {
    return quorumSet.threshold;
}

SlotNum LocalNode::Propose(std::string value) {
#if DEBUG
    printf("[INFO]Value is: %s\n", value.c_str());
#endif
    std::lock_guard<std::mutex> lock(mtx);
    auto i = NewSlot();
    auto b = Ballot{1, std::move(value)};
    printf("Finding Nonce\n");
    auto nonce = generateNonce(&b, i);
    printf("Nonce Found %llu\n", nonce);
    auto m = std::make_shared<PrepareMessage>(id, i, b, Ballot{}, Ballot{}, Ballot{}, quorumSet, 0);
    /* TODO; resending etc */
    SendMessage(m);
    printf("messages sent\n");
    return i;
}

void LocalNode::Propose(std::string value, SlotNum sn) {
    std::lock_guard<std::mutex> lock(mtx);
    auto b = Ballot{1, std::move(value)};
#ifdef VERBOSE
    printf("Finding Nonce\n");
#endif
    auto nonce = generateNonce(&b, sn);
#ifdef VERBOSE
    printf("Nonce Found %llu\n", nonce);
#endif
    auto m = std::make_shared<PrepareMessage>(id, sn, b, Ballot{}, Ballot{}, Ballot{}, quorumSet, 0);
    /* TODO; resending etc */
    SendMessage(m);
#ifdef VERBOSE
    printf("messages sent\n");
#endif
}

SlotNum LocalNode::NewSlot() {
    return maxSlot;
}

void LocalNode::SendMessage(std::shared_ptr<Message> msg) {
#if DEBUG
    printf("[INFO] SendMessage start\n");
#endif
    // TODO : interface with FakeRPC.
    mc->Broadcast(std::move(msg), GetQuorumSet().members);
}

void LocalNode::SendMessageTo(std::shared_ptr<Message> msg, NodeID i) {
    // TODO : interface with FakeRPC.
    mc->Send(std::move(msg), i);
}

bool LocalNode::ReceiveMessage(std::shared_ptr<Message> *msg) {
    bool received = mc->Receive(msg);
    if (received && msg) {

        // PRINT here just to show we got it
#ifdef VERBOSE
        printf("Got a message\n");
#endif
        return true;
    }
    return false;
}

void LocalNode::ProcessMessage(std::shared_ptr<Message> msg) {
    auto slot = msg->getSlot();
    if (log.find(slot) == log.end()) {
        log[slot] = std::make_shared<Slot>(slot, this);
        if (slot > maxSlot) {
            maxSlot = slot;
        }
    }
    log[msg->getSlot()]->handle(msg);
}

void LocalNode::DumpLog() {
    for (auto slot : log) {
        slot.second->Dump();
    }
}


std::pair<std::string, bool> LocalNode::View(SlotNum s) {
    std::lock_guard<std::mutex> lock(mtx);
    try {
        bool b = log.at(s)->GetPhase() == EXTERNALIZE;
        return std::pair<std::string, bool>(log.at(s)->GetValue(), b);
    } catch (const std::out_of_range& e) {
        return std::pair<std::string, bool>("", false);
    }
}
