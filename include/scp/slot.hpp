#ifndef SLOT_H
#define SLOT_H

#include <string>
#include <map>

#include "util/common.hpp"
#include "ballot.hpp"
#include "quorum.hpp"

namespace DISTPROJ {

    class Message;

    class PrepareMessage;

    class FinishMessage;

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


    class Slot {
    private:
        SlotState state;
        Phase phi;
        std::map<NodeID, std::shared_ptr<Message>> M;
        LocalNode* node;


    public:
        Slot(SlotNum id, LocalNode* m);

        void handle(std::shared_ptr<Message> msg);

        Phase GetPhase() { return phi; };

        std::string GetValue() { return state.c.value; };

        std::string Phase_s() {
            const static std::map<Phase, std::string> phase = {{PREPARE,     "Prepare"},
                                                               {FINISH,      "Finish"},
                                                               {EXTERNALIZE, "Externalize"}};
            return phase.at(phi);
        };
    private:
        void handle(std::shared_ptr<PrepareMessage> msg);

        void handle(std::shared_ptr<FinishMessage> msg);

        std::shared_ptr<Message> lastDefined(NodeID n);

        std::shared_ptr<PrepareMessage> Prepare();

        std::shared_ptr<FinishMessage> Finish();

        bool isExceededThreshold(Ballot b);
    };
}

#endif
