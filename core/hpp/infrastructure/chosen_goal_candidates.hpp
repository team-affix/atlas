#ifndef CHOSEN_GOAL_CANDIDATES_HPP
#define CHOSEN_GOAL_CANDIDATES_HPP

#include <optional>
#include <unordered_map>
#include "interfaces/i_try_get_chosen_goal_candidate.hpp"
#include "interfaces/i_set_chosen_goal_candidate.hpp"
#include "interfaces/i_clear_chosen_goal_candidates.hpp"

struct chosen_goal_candidates
    : i_try_get_chosen_goal_candidate
    , i_set_chosen_goal_candidate
    , i_clear_chosen_goal_candidates {
    std::optional<rule_id> try_get(const goal_lineage*) const override;
    void set(const goal_lineage*, rule_id) override;
    void clear() override;
private:
    std::unordered_map<const goal_lineage*, rule_id> by_goal_;
};

#endif
