#ifndef CANDIDATE_HPP
#define CANDIDATE_HPP

#include <cstdint>
#include <unordered_map>
#include "expr.hpp"
#include "lineage.hpp"

struct candidate {
    virtual ~candidate() = default;
    candidate(const resolution_lineage*);
    const expr* copied_head;
    std::unordered_map<uint32_t, uint32_t> tm;
};

#endif
