#ifndef GOAL_CANDIDATE_RULES_HPP
#define GOAL_CANDIDATE_RULES_HPP

#include <unordered_map>
#include "../interfaces/i_get_goal_candidate_rules.hpp"
#include "../interfaces/i_link_goal_candidate.hpp"
#include "../interfaces/i_unlink_goal_candidate.hpp"
#include "../interfaces/i_constrain_goal_candidate_rules.hpp"
#include "rule_set.hpp"

struct goal_candidate_rules
    : i_get_goal_candidate_rules
    , i_link_goal_candidate
    , i_unlink_goal_candidate
    , i_constrain_goal_candidate_rules {
    i_rule_set& get(const goal_lineage*) override;
    const i_rule_set& get(const goal_lineage*) const override;
    void link_goal_candidate(const goal_lineage*, const rule*) override;
    void unlink_goal_candidate(const goal_lineage*, const rule*) override;
    void constrain_goal_candidate_rules(const resolution_lineage*) override;
private:
    std::unordered_map<const goal_lineage*, rule_set> by_goal_;
    rule_set empty_;
};

#endif
