#include <utility>

#include "scp/node.hpp"
#include "scp/message.hpp"
#include "scp/slot.hpp"

using namespace DISTPROJ;

Slot::Slot(SlotNum id, LocalNode* m) : phi(PREPARE), node(m) {
    state.slotNum = id;
    state.b = NILBALLOT;
    state.p = NILBALLOT;
    state.p_ = NILBALLOT;
    state.c = NILBALLOT;
}

std::shared_ptr<PrepareMessage> Slot::Prepare(Quorum qset, NodeID nodeID) {
    auto p = std::make_shared<PrepareMessage>(nodeID, state.slotNum, state.b, state.p, state.p_, state.c, qset, 0);
    return p;
}

std::shared_ptr<FinishMessage> Slot::Finish(Quorum qset, NodeID nodeID) {
    auto p = std::make_shared<FinishMessage>(nodeID, state.slotNum, state.b, qset);
    return p;
}

std::shared_ptr<Message> Slot::lastDefined(NodeID from) {
    std::shared_ptr<Message> last;
    try {
        last = M.at(from);
    } catch (std::out_of_range &e) {
        last = std::make_shared<PrepareMessage>(from, 0, NILBALLOT, NILBALLOT, NILBALLOT, NILBALLOT, Quorum{}, 0);
        M[from] = last;
    }
    return last;
}

HandleReturn Slot::handle(const std::shared_ptr<Message> msg, Quorum qset, NodeID nodeID) {
    printf("11111111111111111111111111111\n");
    HandleReturn handleReturn{};
    handleReturn.shouldSendMsg = false;
    handleReturn.shouldCallBack = false;

    std::shared_ptr<Message> last;
#ifdef VERBOSE
    Dump(nodeID);
#endif
    auto from = msg->from();
    if (qset.members.find(from) != qset.members.end()) {
        last = lastDefined(from);
        if (msg->follows(last)) {
            if (phi == EXTERNALIZE && from != nodeID) {
                handleReturn.messageType = MessageType::FinishMessage_t;
                handleReturn.shouldSendMsg = true;
                handleReturn.shouldCallBack = true;
            } else {
                M[from] = msg;
                switch (msg->type()) {
                    case PrepareMessage_t: {
                        handleReturn = handle(std::static_pointer_cast<PrepareMessage>(msg), qset, nodeID);
                    }
                        break;
                    case FinishMessage_t: {
                        handleReturn = handle(std::static_pointer_cast<FinishMessage>(msg), qset, nodeID);
                    }
                        break;
                    default:
                        exit(EXIT_FAILURE);
                }
            }
        }
    }
#ifdef VERBOSE
    Dump(nodeID);
#endif
    return handleReturn;
}

HandleReturn Slot::handle(const std::shared_ptr<PrepareMessage> msg, Quorum qset, NodeID nodeID) {
    printf("22222222222222222222222222222\n");
#ifdef VERBOSE
    printf("PREPARE\n");
#endif
    HandleReturn handleReturn{};
    handleReturn.shouldSendMsg = false;
    handleReturn.shouldCallBack = false;

    bool returnNow = false;
    // If phase is not prepare, return
    if (phi != PREPARE) {
        // Send 1-to-1 finish message to from node.
    }
    else if (state.b == NILBALLOT) {
        // Definition -- vote:
        //  node v votes for a iff
        //  1) v asserts a is valid/consistent
        //  2) v promises not to vote against a.

        // First case: We've never voted for anything. I.E. b = 0;
        // Vote for b but don't accept yet.
        state.b.value = msg->GetB().value;
        state.b.num = 1;
        // Send out vote for b.
        handleReturn.shouldSendMsg = true;
        handleReturn.shouldCallBack = false;
        handleReturn.messageType = MessageType::PrepareMessage_t;
    } else {
        // if( true /* && a message allows v to accept that new ballots are prepared by either of accepts 2 criteria */) {
        // if prepared ballot then set p

        // Definition -- Accept:
        //  node v accepts statement a (the value in b) iff it has never accepted
        //  a contradicting statment and
        //  1) there exists U s.t. node v is in U and everyone in U has voted for
        //    or accepted a. OR
        //  2) Each member of a v-blocking set claims to accept a.

        // Check that we haven't accepted a contradicting statement.

        // NOTE : the > operator does not accomplish the logic below.
        if (compatible(msg->GetB(), state.p) || state.p == NILBALLOT) {
            // Now check that one of our quorum slices has all voted for or
            // accepted b.
            auto b_voted_or_accepted = qset.threshold;
            for (auto kp : M) {
                auto m = kp.second;
                switch (m->type()) {
                    case FinishMessage_t:
                        if ((std::static_pointer_cast<FinishMessage>(m))->GetB() == msg->GetB()) {
                            b_voted_or_accepted--;
                        }
                        break;
                    case PrepareMessage_t:
                        if ((std::static_pointer_cast<PrepareMessage>(m))->GetB() == msg->GetB() ||
                            (std::static_pointer_cast<PrepareMessage>(m))->GetP() == msg->GetB()) {
                            b_voted_or_accepted--;
                        }
                        break;
                }
                // This can be moved outside of the for loop -- this let's it
                // duck out as soon as threshold crossed
                if (b_voted_or_accepted == 0) {
                    state.p = state.b;
//                    returnNow = true;
                    handleReturn.shouldSendMsg = true;
                    handleReturn.shouldCallBack = false;
                    handleReturn.messageType = MessageType::PrepareMessage_t;
                    break;
                }
            }
        } else {
            // Statement contradicted. Check for v-blocking.
            auto b_vblock_vote = qset.threshold;
            for (auto kp : M) {
                auto m = kp.second;
                switch (m->type()) {
                    case FinishMessage_t:
                        if ((std::static_pointer_cast<FinishMessage>(m))->GetB() == msg->GetB()) {
                            b_vblock_vote--;
                        }
                        break;
                    case PrepareMessage_t:
                        if ((std::static_pointer_cast<PrepareMessage>(m))->GetP() == msg->GetB()) {
                            b_vblock_vote--;
                        }
                        break;
                }

                if (b_vblock_vote == 0) {
                    // v-blocking set found so vote the contradicting ballot.
                    state.p_ = state.p;
                    state.p = NILBALLOT;
                    state.b.value = msg->GetB().value;
                    state.b.num += 1;
//                    returnNow = true;
                    handleReturn.shouldSendMsg = true;
                    handleReturn.shouldCallBack = false;
                    handleReturn.messageType = MessageType::PrepareMessage_t;
                    break;
                }
            }

        }

        // If a c ballot exists but p >!~ c or p_ >!~ c, clear c.
        if (state.c.num != 0 && (state.p > state.c || state.p_ > state.c)) {
            state.c = NILBALLOT;
//            returnNow = true;
            handleReturn.shouldSendMsg = true;
            handleReturn.shouldCallBack = false;
            handleReturn.messageType = MessageType::PrepareMessage_t;
        }

//        if (returnNow) {
//            node->SendMessage(Prepare(qset, nodeID));
//        }

        if (state.b != state.c && state.b == state.p /* V confirms b is prepared */ ) {
            auto b_prepared = qset.threshold;
            for (auto kp : M) {
                auto m = kp.second;
                switch (m->type()) {
                    case FinishMessage_t:
                        if ((std::static_pointer_cast<FinishMessage>(m))->GetB() == state.p) {
                            b_prepared--;
                        }
                        break;
                    case PrepareMessage_t:
                        if ((std::static_pointer_cast<PrepareMessage>(m))->GetB() == state.p) {
                            b_prepared--;
                        }
                        break;
                }

                if (b_prepared == 0) {
                    state.c = state.b;
//                    node->SendMessage(Finish(qset, nodeID));
                    handleReturn.shouldSendMsg = true;
                    handleReturn.shouldCallBack = false;
                    handleReturn.messageType = MessageType::FinishMessage_t;
                    break;
                }
            }

        }
    }
    return handleReturn;
}

HandleReturn Slot::handle(const std::shared_ptr<FinishMessage> msg, Quorum qset, NodeID nodeID) {
    printf("333333333333333333333333333\n");
    HandleReturn handleReturn{};
    handleReturn.shouldSendMsg = false;
    handleReturn.shouldCallBack = false;
#ifdef VERBOSE
    printf("Finish\n");
#endif
    // Finish message implies every statement implied by Prepare v i b b 0 b D.
    auto p = std::make_shared<PrepareMessage>(nodeID, state.slotNum, state.b, state.b, NILBALLOT, state.b, qset, 0);
    // handle(p);
    if (phi == PREPARE && state.b == state.p && state.b == state.c && msg->GetB() == state.b) { // RULE 3
        phi = FINISH;
        // TODO (JHH) : Figure what if anything needs to happen here.
        //return; /// ???????????????????>?>?????????????????questionmark??
    }
    if (phi == FINISH && state.b == state.p && state.b == state.c && msg->GetB() == state.b) { // RULE 4
        // Check that this node ~confirms~ b.
        auto b_commit = qset.threshold;
        for (auto kp : M) {
            auto m = kp.second;
            switch (m->type()) {
                case FinishMessage_t:
                    if ((std::static_pointer_cast<FinishMessage>(m))->GetB() ==
                        state.c) { // Finish -> b == Prepare -> c
                        b_commit--;
                    }
                    break;
                case PrepareMessage_t:
                    if ((std::static_pointer_cast<PrepareMessage>(m))->GetC() == state.c) {
                        b_commit--;
                    }
                    break;
            }
#ifdef VERBOSE
            printf("Externalizing need %d", b_commit);
#endif
            if (b_commit == 0) {
                phi = EXTERNALIZE;
                break;
            }
        }
    } else {
        // TODO : Might need to check for a v-blocking set and go back into the
        // prepare state.
    }
    return handleReturn;
}

// Dump state / received message inforamtion.
void Slot::Dump(NodeID nodeID) {
    printf("Dumping id: %llu\n    slot: %u, b: %d, p: %d, p_: %d, c:%d \n%s\n, Phase %s\n", nodeID,
           static_cast<unsigned int>(state.slotNum), state.b.num, state.p.num, state.p_.num, state.c.num,
           state.c.value.c_str(),
           Phase_s().c_str());
}

