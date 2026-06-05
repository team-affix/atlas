#ifndef SRT_ACTIVE_GOALS_HPP
#define SRT_ACTIVE_GOALS_HPP

#include <set>
#include <unordered_set>
#include "infrastructure/series_reduced_tree.hpp"
#include "interfaces/i_insert_active_goal.hpp"
#include "interfaces/i_erase_active_goal.hpp"
#include "interfaces/i_is_active_goal.hpp"
#include "interfaces/i_active_goals_size.hpp"
#include "interfaces/i_check_active_goals_empty.hpp"
#include "interfaces/i_clear_active_goals.hpp"

struct srt_active_goals
    : i_insert_active_goal
    , i_erase_active_goal
    , i_is_active_goal
    , i_active_goals_size
    , i_check_active_goals_empty
    , i_clear_active_goals {
    void insert_active_goal(const goal_lineage*) override;
    void erase_active_goal(const goal_lineage*) override;
    bool is_active_goal(const goal_lineage*) const override;
    size_t active_goals_size() const override;
    bool empty() const override;
    void clear_active_goals() override;
private:
    series_reduced_tree<const goal_lineage*> tree_;
};

#endif
