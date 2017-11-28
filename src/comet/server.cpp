#include "util/common.hpp"
#include "comet/server.hpp"

using namespace DISTPROJ;

int node_id = 0;

ServerKV::ServerKV(std::shared_ptr<RPCLayer> rpc, float _quorumThresholdRatio)
        : quorumThresholdRatio(_quorumThresholdRatio) {

    node = new LocalNode(node_id++, rpc, Quorum{});
//    node = new LocalNode(nrand(), rpc, Quorum{});
    curSlot = 0;

    node->Start();
}

void ServerKV::Put(PutArgs &args, PutReply &reply) {
    std::lock_guard<std::mutex> lock(mtx);
    Operation op;
    op.id = args.id;
    op.opType = PUT;
    op.key = args.key;
    op.value = args.value;

    ApplyOperation(op);
    reply.err = "OK";
}

void ServerKV::Get(GetArgs &args, GetReply &reply) {
    std::lock_guard<std::mutex> lock(mtx);

    Operation op;
    op.id = args.id;
    op.opType = GET;
    op.key = args.key;

    reply.value = ApplyOperation(op);
    reply.err = "OK";
}

std::string ServerKV::ApplyOperation(Operation &op) {
    while (true) {
        // Check if we've seen this operation.
        {
            auto it = seen.find(op.id);
            if (it != seen.end()) {
                return it->second;
            }
        }

        Operation decidedOp;
        auto newSlot = curSlot + 1;
        auto p = node->View(newSlot);
        if (p.second) {
            std::istringstream ss;
            ss.str(p.first);
            {
                cereal::JSONInputArchive archive(ss);
                archive(decidedOp);
            }
        } else {
            // Propose.
            std::ostringstream ss;
            {
                cereal::JSONOutputArchive archive(ss);
                archive(op);
            }
            node->Propose(ss.str(), newSlot);

            // Wait for a decision.
            while (!p.second) {
                p = node->View(newSlot);
            }
            std::istringstream iss;
            iss.str(p.first);
            {
                cereal::JSONInputArchive archive(iss);
                archive(decidedOp);
            }
        }

        // Apply operation.
        std::string result;
        switch (decidedOp.opType) {
            case GET:
                seen[decidedOp.id] = db[decidedOp.key];
                break;
            case PUT:
                seen[decidedOp.id] = "";
                db[decidedOp.key] = decidedOp.value;
                break;
        }
        curSlot++;

        // Check termination condition.
        if (decidedOp.id == op.id) {
            return seen[decidedOp.id];
        }
    }
}

void ServerKV::AddPeer(NodeID peer) {
    node->AddNodeToQuorum(peer);
    ResizeThreshold();
}

void ServerKV::AddPeers(std::set<NodeID> peers) {
    for (auto n : peers) {
        AddPeer(n);
    }
}

void ServerKV::RemovePeer(NodeID peer) {
    node->RemoveNodeFromQuorum(peer);
    ResizeThreshold();
}

void ServerKV::RemovePeers(std::set<NodeID> peers) {
    for (auto n : peers) {
        RemovePeer(n);
    }
}
