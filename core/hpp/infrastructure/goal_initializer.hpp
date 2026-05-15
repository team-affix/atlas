#ifndef GOAL_INITIALIZER_HPP
#define GOAL_INITIALIZER_HPP

#include "../domain/interfaces/i_goal_initializer.hpp"
#include "../domain/interfaces/i_database.hpp"
#include "../domain/interfaces/i_initial_goal_exprs.hpp"
#include "../domain/interfaces/i_frontier.hpp"
#include "../domain/interfaces/i_copier.hpp"

struct goal_initializer : i_goal_initializer {
    goal_initializer();
    void seed_expansion(const resolution_lineage*) override;
    void initialize(const goal_lineage*) override;
private:
    const i_database& db_;
    const i_initial_goal_exprs& initial_goal_exprs_;
    const i_frontier& frontier_;
    const i_copier& copier_;
    
    std::unordered_map<uint32_t, uint32_t>* tm_;
    const std::vector<const expr*>* chosen_rule_body_;
};

#endif
