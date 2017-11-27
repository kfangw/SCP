#ifndef NODE_H
#define NODE_H

#include <set>
#include <map>
#include <cstdlib>
#include <cstdint>
#include <thread>
#include <mutex>

#include "quorum.hpp"

namespace DISTPROJ {

    class RPCLayer;

    class Message;

    class Slot;

    class MessageClient;

    class Node {

    public:
        Node(NodeID _id, std::shared_ptr<RPCLayer> _rpc);

        Node(NodeID _id, std::shared_ptr<RPCLayer> _rpc, Quorum _quorumSet);

        NodeID GetNodeID();

        Quorum GetQuorumSet();

#if USED
        void PrintQuorumSet();
#endif

    protected:
        NodeID id;
        std::shared_ptr<RPCLayer> rpc;
        Quorum quorumSet;
        std::thread *t;

    };

    class LocalNode : public Node {

    public:
        LocalNode(NodeID _id, std::shared_ptr<RPCLayer> _rpc);

        LocalNode(NodeID _id, std::shared_ptr<RPCLayer> _rpc, Quorum _quorumSet);

        void AddKnownNode(NodeID v);

        void AddNodeToQuorum(NodeID v);

        void RemoveNodeFromQuorum(NodeID v);

        void UpdateQurorum(Quorum quorumSet);

        unsigned long QuorumSize();

        void SetThreshold(unsigned long t);

        unsigned long GetThreshold();

        void Start();

        SlotNum Propose(std::string value);

        void Propose(std::string value, SlotNum sn);

        void SendMessage(std::shared_ptr<Message> msg);

        void SendMessageTo(std::shared_ptr<Message> msg, NodeID i);

        bool ReceiveMessage(std::shared_ptr<Message> *msg);

        void ProcessMessage(std::shared_ptr<Message> msg);

        std::pair<std::string, bool> View(SlotNum s);;

        void IncrementMaxSlot() { maxSlot++; };

        SlotNum GetMaxSlot() { return maxSlot; };

    private:
        SlotNum maxSlot;
        std::mutex mtx;

        SlotNum NewSlot(); // only one of these can run at a time
        void Tick();

        std::map<SlotNum, std::shared_ptr<Slot>> slotLog;
        std::set<NodeID> knownNodes;
        MessageClient *mc;

    };

}

#endif
