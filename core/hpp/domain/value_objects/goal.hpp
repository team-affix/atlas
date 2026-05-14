#ifndef GOAL_HPP
#define GOAL_HPP

#include <memory>
#include <unordered_map>
#include "candidate.hpp"
#include "expr.hpp"
#include "lineage.hpp"
#include "../interfaces/i_database.hpp"

struct goal {
    virtual ~goal() = default;
    goal(const goal_lineage*);
    virtual void choose(size_t);
    const expr* e;
    std::unordered_map<size_t, std::unique_ptr<candidate>> candidates;
    const std::vector<const expr*>* chosen_rule_body;
    std::unordered_map<uint32_t, uint32_t>* chosen_rule_tm;
private:
    const i_database& db_;
};

#endif
