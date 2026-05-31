#ifndef GOAL_CANDIDATE_RULES_HPP
#define GOAL_CANDIDATE_RULES_HPP

#include <unordered_map>
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"
#include "interfaces/i_insert_goal_candidates.hpp"
#include "interfaces/i_link_goal_candidate.hpp"
#include "interfaces/i_unlink_goal_candidate.hpp"
#include "interfaces/i_erase_goal_candidates.hpp"
#include "interfaces/i_clear_goal_candidate_rule_ids.hpp"
#include "rule_id_set.hpp"

struct goal_candidate_rules
    : i_get_goal_candidate_rule_ids
    , i_insert_goal_candidates
    , i_link_goal_candidate
    , i_unlink_goal_candidate
    , i_erase_goal_candidates
    , i_clear_goal_candidate_rule_ids {
    i_rule_id_set& get(const goal_lineage*) override;
    const i_rule_id_set& get(const goal_lineage*) const override;
    void insert(const goal_lineage*) override;
    void link_goal_candidate(const goal_lineage*, rule_id) override;
    void unlink_goal_candidate(const goal_lineage*, rule_id) override;
    void erase(const goal_lineage*) override;
    void clear_goal_candidate_rule_ids() override;
private:
    std::unordered_map<const goal_lineage*, rule_id_set> by_goal_;
};

#endif
