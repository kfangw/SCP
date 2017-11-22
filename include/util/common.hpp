
#ifndef TYPES_H

#define TYPES_H

#include <cstdint>
#include <string>
#include <random>
#include <cereal/archives/json.hpp>

typedef uint64_t NodeID;
typedef std::string OpID;
typedef uint64_t SlotNum;
typedef uint64_t Nonce;
typedef uint64_t Version;


inline uint64_t nrand() {
    // Generate node id.
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<NodeID> dist(0, ~0);

    return dist(gen);
}

#endif
