#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>

#include "ballot.hpp"
#include "quorum.hpp"
#include "util/common.hpp"
#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/polymorphic.hpp>

namespace DISTPROJ {

    enum MessageType {
        FinishMessage_t = 0, PrepareMessage_t = 1
    };

    class Slot;

    class FinishMessage;

    class PrepareMessage;

    class Message {
    private:
        MessageType t;

    public:
        Message(MessageType t, NodeID _v, SlotNum _slotID, Quorum _d, Ballot _b)
                : t(t), v(_v), slotID(_slotID), d(_d), b(_b){};

        MessageType type() { return t; };

        SlotNum getSlot() { return slotID; };
        NodeID from() { return v; };
        const Ballot &GetB() const { return b; }

        virtual bool follows(std::shared_ptr<Message> x) = 0;

    protected:
        NodeID v;
        SlotNum slotID;
        Quorum d = Quorum{};
        Ballot b = NILBALLOT;

    };

    class PrepareMessage : public Message {

    public:
        PrepareMessage() : PrepareMessage(0, 0, NILBALLOT, NILBALLOT, NILBALLOT, NILBALLOT, Quorum{}, 0) {};

        PrepareMessage(NodeID _v, SlotNum _slotID, Ballot _b, Ballot _p,
                       Ballot _p_, Ballot _c, Quorum _d, Nonce n)
                : Message(MessageType::PrepareMessage_t, _v, _slotID, _d, _b), p(_p), p_(_p_), c(_c), nonce(n) {};

        template<class Archive>
        void serialize(Archive &archive) {
            archive(CEREAL_NVP(v), CEREAL_NVP(slotID), CEREAL_NVP(b),
                    CEREAL_NVP(p), CEREAL_NVP(p_), CEREAL_NVP(c),
                    CEREAL_NVP(d), CEREAL_NVP(nonce));
        };

        bool follows(std::shared_ptr<Message> x) override ;

    public:
        const Ballot &GetP() const { return p; }
        const Ballot &GetP_() const { return p_; }
        const Ballot &GetC() const { return c; }

    private:
        Ballot p = NILBALLOT;
        Ballot p_ = NILBALLOT;
        Ballot c = NILBALLOT;
        Nonce nonce = 0;

    };

    class FinishMessage : public Message {

    public:
        FinishMessage() : FinishMessage(0, 0, NILBALLOT, Quorum{}) {};

        FinishMessage(NodeID _v, SlotNum _slotID, Ballot _b, Quorum _d)
                : Message(MessageType::FinishMessage_t, _v, _slotID, _d, _b) {};


        template<class Archive>
        void serialize(Archive &archive) {
            archive(CEREAL_NVP(v), CEREAL_NVP(slotID), CEREAL_NVP(b),
                    CEREAL_NVP(d)); // serialize things by passing them to the archive
        };

        bool follows(std::shared_ptr<Message> x) override ;

    };

}

CEREAL_REGISTER_POLYMORPHIC_RELATION(DISTPROJ::Message, DISTPROJ::PrepareMessage);
CEREAL_REGISTER_POLYMORPHIC_RELATION(DISTPROJ::Message, DISTPROJ::FinishMessage);
CEREAL_REGISTER_TYPE_WITH_NAME(DISTPROJ::FinishMessage, "Finish");
CEREAL_REGISTER_TYPE_WITH_NAME(DISTPROJ::PrepareMessage, "Prepare");

#endif
