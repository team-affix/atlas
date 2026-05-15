#ifndef CANDIDATE_HPP
#define CANDIDATE_HPP

#include <cstdint>
#include <unordered_map>
#include "expr.hpp"

struct candidate {
    virtual ~candidate() = default;
    const expr* copied_head;
    std::unordered_map<uint32_t, uint32_t> tm;
};

#endif
