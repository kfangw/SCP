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

std::shared_ptr<PrepareMessage> Slot::Prepare() {
    auto p = std::make_shared<PrepareMessage>(node->GetNodeID(), state.slotNum, state.b, state.p, state.p_, state.c,
                                              node->GetQuorumSet(), 0);
    return p;
}

std::shared_ptr<FinishMessage> Slot::Finish() {
    auto p = std::make_shared<FinishMessage>(node->GetNodeID(), state.slotNum, state.b, node->GetQuorumSet());
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

void Slot::handle(std::shared_ptr<Message> _msg) {
    auto from = _msg->from();
    /**
     * If the node, sent message, is not registered in this node, ignore it
     */
    if (node->GetQuorumSet().members.find(from) == node->GetQuorumSet().members.end())
        return;
    /**
     * If previous message's number is LT current, ignore it
     */
    if ( !(_msg->follows(lastDefined(from))))
        return;

    /**
     * If current state is EXTERNALIZE, ack back to the node which sent message
     */
    if (phi == EXTERNALIZE && from != node->GetNodeID()) {
        node->SendMessageTo(Finish(), from);
        return;
    }

    /**
     * Handle message acording to its type
     */
    if (_msg->type() == MessageType::PrepareMessage_t)
        handle(std::static_pointer_cast<PrepareMessage>(_msg));

    if (_msg->type() == MessageType::FinishMessage_t)
        handle(std::static_pointer_cast<FinishMessage>(_msg));

    /**
     * Record current message
     */
    M[from] = _msg;
}

void Slot::handle(std::shared_ptr<PrepareMessage> msg) {
    /**
     * Just sync thread of nodes
     */
    if (phi != PREPARE) {
        return;
    }
    /** 1.
     * Recieved first ballot.
     * Copy contents of the message to STATE-B
     */
    if (state.b == NILBALLOT) {
        state.b.value = msg->GetB().value;
        state.b.num = 1;
        node->SendMessage(Prepare());
        return;
    }
    /** 2.
     * If Already Commited,
     * ignore previous commit and restart
     */
    if (state.c != NILBALLOT &&
        ( state.p > state.c ||
          state.p_ > state.c)) {
        state.c = NILBALLOT;
        node->SendMessage(Prepare());
        return;
    }

    /** 3 ~ 5
     * Recieved acknowledge of prepare message (Actually, those are not distinguishable)
     * If the threshold exceeded,
     */
    if (isExceededThreshold(msg->GetB())){
        if (compatible(msg->GetB(), state.p) || state.p == NILBALLOT) {
            /** 3.
             * If already in STATE-P or not yet recieved prepare message
             * Copy contents of the message to STATE-P
             * And send prepare message
             */
            state.p = state.b; node->SendMessage(Prepare());
            if (state.b != state.c){
                /** 4.
                 * If not yet committed
                 * Copy contents of the message to STATE-C
                 * And send finish message
                 */
                phi = FINISH; state.c = state.b; node->SendMessage(Finish());
            }
        } else {
            /**
             * 5.
             * If current STATE-P is not compatible with the message,
             * SET STATE-P to STATE-P' and re-invoke consensus from the beginning(send message P)
             */
            state.p_ = state.p;
            state.p = NILBALLOT;
            state.b.value = msg->GetB().value;
            state.b.num += 1;
            node->SendMessage(Prepare());
        }
    }
}

void Slot::handle(std::shared_ptr<FinishMessage> msg) {
    if (isExceededThreshold(msg->GetB())){
        if (phi == FINISH &&
            state.b == state.p &&
            state.b == state.c &&
            state.b == msg->GetB()) {
            phi = EXTERNALIZE;
        }
    }
}

bool Slot::isExceededThreshold(Ballot b){
    auto vote_threshold = node->GetQuorumSet().threshold;
    for (auto kp : M) {
        auto pastMessage = kp.second;
        if (pastMessage->GetB() == b) {
            vote_threshold--;
            if (vote_threshold == 0)
                return true;
        }
    }
    return false;
}
