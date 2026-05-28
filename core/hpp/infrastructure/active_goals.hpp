#ifndef ACTIVE_GOALS_HPP
#define ACTIVE_GOALS_HPP

#include <unordered_set>
#include "interfaces/i_insert_active_goal.hpp"
#include "interfaces/i_erase_active_goal.hpp"
#include "interfaces/i_is_active_goal.hpp"
#include "interfaces/i_iterate_active_goals.hpp"
#include "interfaces/i_active_goals_size.hpp"
#include "interfaces/i_check_active_goals_empty.hpp"

struct active_goals
    : i_insert_active_goal
    , i_erase_active_goal
    , i_is_active_goal
    , i_iterate_active_goals
    , i_active_goals_size
    , i_check_active_goals_empty {
    void insert_active_goal(const goal_lineage*) override;
    void erase_active_goal(const goal_lineage*) override;
    bool is_active_goal(const goal_lineage*) const override;
    state_machine<const goal_lineage*> iterate_active_goals() const override;
    size_t active_goals_size() const override;
    bool empty() const override;
private:
    std::unordered_set<const goal_lineage*> goals_;
};

#endif
