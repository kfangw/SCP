#ifndef SLOT_H
#define SLOT_H

#include <string>
#include <map>

#include "util/common.hpp"
#include "ballot.hpp"
#include "quorum.hpp"
#include "message.hpp"

namespace DISTPROJ {

//    class Message;
//
//    class PrepareMessage;
//
//    class FinishMessage;

    class LocalNode;

    enum Phase {
        PREPARE, FINISH, EXTERNALIZE
    };

    struct SlotState {
        Ballot b;
        Ballot p;
        Ballot p_;
        Ballot c;
        SlotNum slotNum;
    };

    struct HandleReturn {
        bool shouldSendMsg;
        bool shouldCallBack;
        MessageType messageType;
    };


    class Slot {

    public:
        Slot(SlotNum id, LocalNode* m);

        HandleReturn handle(std::shared_ptr<Message> msg, Quorum qset, NodeID nodeID);

        // Dump state / received message inforamtion.
        void Dump(NodeID nodeID);

        Phase GetPhase() { return phi; };

        std::string GetValue() { return state.c.value; };

        std::shared_ptr<PrepareMessage> Prepare(Quorum qset, NodeID nodeID);

        std::shared_ptr<FinishMessage> Finish(Quorum qset, NodeID nodeID);

        std::string Phase_s() {

            const static std::map<Phase, std::string> phase = {{PREPARE,     "Prepare"},
                                                               {FINISH,      "Finish"},
                                                               {EXTERNALIZE, "Externalize"}};
            return phase.at(phi);
        };
    private:
        HandleReturn handle(std::shared_ptr<PrepareMessage> msg, Quorum qset, NodeID nodeID);

        HandleReturn handle(std::shared_ptr<FinishMessage> msg, Quorum qset, NodeID nodeID);

        std::shared_ptr<Message> lastDefined(NodeID n);

        SlotState state;
        Phase phi;
        std::map<NodeID, std::shared_ptr<Message>> M;
        LocalNode* node;


    };
}

#endif
