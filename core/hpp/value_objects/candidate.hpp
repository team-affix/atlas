#ifndef CANDIDATE_HPP
#define CANDIDATE_HPP

#include <cstdint>
#include <unordered_map>

struct candidate {
    virtual ~candidate() = default;
    std::unordered_map<uint32_t, uint32_t> tm;
};

#endif
