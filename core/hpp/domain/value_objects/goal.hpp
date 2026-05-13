#ifndef GOAL_HPP
#define GOAL_HPP

#include <memory>
#include "candidate.hpp"
#include "expr.hpp"
#include "../interfaces/i_set.hpp"
#include "../interfaces/i_map.hpp"

struct goal {
    virtual ~goal() = default;
    const expr* e;
    std::unique_ptr<i_set<uint32_t>> e_reps;
    std::unique_ptr<i_map<size_t, std::unique_ptr<candidate>>> candidates;
};

#endif
