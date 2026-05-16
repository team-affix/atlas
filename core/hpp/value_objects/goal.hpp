#ifndef GOAL_HPP
#define GOAL_HPP

#include <memory>
#include <unordered_map>
#include "candidate.hpp"
#include "expr.hpp"

struct goal {
    virtual ~goal() = default;
    const expr* e;
    std::unordered_map<size_t, std::unique_ptr<candidate>> candidates;
};

#endif
