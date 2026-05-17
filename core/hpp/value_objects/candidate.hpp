#ifndef CANDIDATE_HPP
#define CANDIDATE_HPP

#include <cstdint>
#include <unordered_map>
#include "unify_head.hpp"

struct candidate {
    virtual ~candidate() = default;
    candidate(
        const std::vector<const expr*>& rule_body,
        std::unordered_map<uint32_t, uint32_t> tm,
        unify_head unify_head);
    const std::vector<const expr*>& rule_body;
    std::unordered_map<uint32_t, uint32_t> tm;
    unify_head unify_head;
};

#endif
