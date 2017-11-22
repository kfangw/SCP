#include <utility>

#include "asteroid/stellarkv.hpp"

using namespace DISTPROJ;
using namespace std;

StellarKV::StellarKV(shared_ptr<RPCLayer> rpc, float tr) : threshold_ratio(tr) {
    node = new LocalNode(nrand(), std::move(rpc), Quorum{});
    slot = 0;
    node->Start();
    t = new thread(&StellarKV::Tick, this);
}

NodeID StellarKV::GetNodeID() {
#ifdef PRINTFUNC
    printf("[FUNCTION] %s\n, ", __PRETTY_FUNCTION__);
#endif
    return node->GetNodeID();
}

void StellarKV::Tick() {
#ifdef PRINTFUNC
    printf("[FUNCTION] %s\n, ", __PRETTY_FUNCTION__);
#endif

    for (;; std::this_thread::sleep_for(std::chrono::milliseconds(10))) {
        {
            lock_guard<mutex> lock(mtx);
            auto p = node->View(node->GetMaxSlot());
            if (p.second) {
                shared_ptr<PutMessage> m;
                std::istringstream ss;
                ss.str(p.first);
                {
                    cereal::JSONInputArchive archive(ss);
                    archive(m);
                }
                m->apply(&log);
                node->IncrementMaxSlot();
            }
        }

    }
}

pair<pair<Version, string>, bool> StellarKV::Get(string k) {
#ifdef PRINTFUNC
    printf("[FUNCTION] %s\n, ", __PRETTY_FUNCTION__);
#endif
    lock_guard<mutex> lock(mtx);
    try {
        return pair<pair<Version, string>, bool>(log.at(k), true);
    } catch (out_of_range& e) {
        auto s = pair<Version, string>(0, "");
        return pair<pair<Version, string>, bool>(s, false);

    }
}

void StellarKV::Put(string k, string val) {
#ifdef PRINTFUNC
    printf("[FUNCTION] %s\n, ", __PRETTY_FUNCTION__);
#endif
    lock_guard<mutex> lock(mtx);
    Version v;
    try {
        v = log.at(k).first;
    } catch (out_of_range& e) {
        v = 0;
    }
    ostringstream ss;
    auto putmessage = make_shared<PutMessage>(k, val, v + 1);
    {
        cereal::JSONOutputArchive archive(ss);
        archive(CEREAL_NVP(putmessage));
    }
    SlotNum maybeSlot;
    auto waiting = true;
    while (waiting) {
        maybeSlot = node->Propose(ss.str());
        for (auto i = 0; i < 10; i++) {
            auto p = node->View(maybeSlot);
            printf("STATE %d %s\n", p.second, putmessage->Value.c_str());

            if (p.second) {
                shared_ptr<PutMessage> m;
                std::istringstream iss;
                iss.str(p.first);

                {
                    cereal::JSONInputArchive archive(iss);
                    archive(m);
                }
                if (*m == *putmessage) {

                    waiting = false;
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

    }
}

int StellarKV::GetThreshold() {
#ifdef PRINTFUNC
    printf("[FUNCTION] %s\n, ", __PRETTY_FUNCTION__);
#endif
    return node->GetThreshold();
}

void StellarKV::AddPeer(NodeID n) {
#ifdef PRINTFUNC
    printf("[FUNCTION] %s\n, ", __PRETTY_FUNCTION__);
#endif
    node->AddNodeToQuorum(n);
    node->SetThreshold((unsigned long) (node->QuorumSize() * threshold_ratio));
}

void StellarKV::AddPeers(set<NodeID> nodes) {
#ifdef PRINTFUNC
    printf("[FUNCTION] %s\n, ", __PRETTY_FUNCTION__);
#endif

    for (auto n : nodes) {
        AddPeer(n);
    }
}

void StellarKV::RemovePeer(NodeID n) {
#ifdef PRINTFUNC
    printf("[FUNCTION] %s\n, ", __PRETTY_FUNCTION__);
#endif
    node->RemoveNodeFromQuorum(n);
    node->SetThreshold((unsigned long)(node->QuorumSize() * threshold_ratio));
}


void StellarKV::RemovePeers(set<NodeID> nodes) {
#ifdef PRINTFUNC
    printf("[FUNCTION] %s\n, ", __PRETTY_FUNCTION__);
#endif
    for (auto n : nodes) {
        RemovePeer(n);
    }
}

void PutMessage::apply(std::map<std::string, std::pair<Version, std::string>> *log) {
    try {
        Version v_ = (*log).at(Key).first;
        if (v_ < v) {
            (*log)[Key] = std::pair<Version, std::string>(v, Value);
        }
    } catch (std::out_of_range& e) {
        (*log)[Key] = std::pair<Version, std::string>(v, Value);
    }
}
