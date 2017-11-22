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

    public:
        Slot(SlotNum id, LocalNode* m);

        void handle(std::shared_ptr<Message> msg);

        // Dump state / received message inforamtion.
        void Dump();

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

        void lastDefined(NodeID n, std::shared_ptr<Message> *m);

        SlotState state;
        Phase phi;
        std::map<NodeID, std::shared_ptr<Message>> M;
        LocalNode* node;

        std::shared_ptr<PrepareMessage> Prepare();

        std::shared_ptr<FinishMessage> Finish();

    };
}

#endif
