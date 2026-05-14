#ifndef GOAL_HPP
#define GOAL_HPP

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "candidate.hpp"
#include "expr.hpp"

struct goal {
    virtual ~goal() = default;
    const expr* e;
    std::unordered_set<uint32_t> e_reps;
    std::unordered_map<size_t, std::unique_ptr<candidate>> candidates;
};

#endif
