#ifndef UNIT_GOALS_HPP
#define UNIT_GOALS_HPP

#include <vector>
#include "../interfaces/i_push_unit_goal.hpp"
#include "../interfaces/i_pop_unit_goal.hpp"
#include "../interfaces/i_check_unit_goals_empty.hpp"
#include "../interfaces/i_clear_unit_goals.hpp"

struct unit_goals
    : i_push_unit_goal
    , i_pop_unit_goal
    , i_check_unit_goals_empty
    , i_clear_unit_goals {
    void push(const goal_lineage*) override;
    const goal_lineage* pop() override;
    bool empty() const override;
    void clear() override;
private:
    std::vector<const goal_lineage*> queue_;
};

#endif
