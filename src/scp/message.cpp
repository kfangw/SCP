#include "scp/message.hpp"
#include "scp/slot.hpp"


using namespace DISTPROJ;

bool FinishMessage::follows(std::shared_ptr<Message> x) {
    /**
     * Check Current Message's number is GTE past Message
     */
    auto m = std::static_pointer_cast<FinishMessage>(x);
    switch (x->type()) {
        case FinishMessage_t:
            return b.num > m->b.num;
        case PrepareMessage_t:
            return true;
        default:
            return true;
    }
}


bool PrepareMessage::follows(std::shared_ptr<Message> x) {
    /**
     * Check Current Message's number is GTE past Message
     */
    auto m = std::static_pointer_cast<PrepareMessage>(x);
    auto first = b.num > m->b.num;
    auto first_continue = b.num == m->b.num;
    auto second = p.num > m->p.num;
    auto second_continue = p.num == m->p.num;
    auto third = p_.num > m->p_.num;
    auto third_continue = p_.num == m->p_.num;
    auto fourth = c.num > m->c.num;

    switch (x->type()) {
        case FinishMessage_t:
            return false;
        case PrepareMessage_t:
            return first || (first_continue &&
                             (second || (second_continue && (third || (third_continue && fourth))))); // See SCP pg 29
        default:
            return true;
    }
}



