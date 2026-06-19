#ifndef I_TRY_GET_CHOSEN_GOAL_CANDIDATE_HPP
#define I_TRY_GET_CHOSEN_GOAL_CANDIDATE_HPP

#include <optional>
#include "value_objects/lineage.hpp"

struct i_try_get_chosen_goal_candidate {
    virtual ~i_try_get_chosen_goal_candidate() = default;
    virtual std::optional<rule_id> try_get(const goal_lineage*) const = 0;
};

#endif
