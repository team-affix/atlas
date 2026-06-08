#ifndef RA_ACTIVE_GOALS_HPP
#define RA_ACTIVE_GOALS_HPP

#include <unordered_map>
#include <vector>
#include "interfaces/i_insert_active_goal.hpp"
#include "interfaces/i_erase_active_goal.hpp"
#include "interfaces/i_is_active_goal.hpp"
#include "interfaces/i_active_goals_size.hpp"
#include "interfaces/i_check_active_goals_empty.hpp"
#include "interfaces/i_clear_active_goals.hpp"
#include "interfaces/i_random_access.hpp"

struct ra_active_goals
    : i_insert_active_goal
    , i_erase_active_goal
    , i_is_active_goal
    , i_active_goals_size
    , i_check_active_goals_empty
    , i_clear_active_goals
    , i_random_access<const goal_lineage*> {
    void insert_active_goal(const goal_lineage*) override;
    void erase_active_goal(const goal_lineage*) override;
    bool is_active_goal(const goal_lineage*) const override;
    size_t active_goals_size() const override;
    bool empty() const override;
    void clear_active_goals() override;
    const goal_lineage* select(size_t index) const override;
private:
    std::unordered_map<const goal_lineage*, size_t> index_;
    std::vector<const goal_lineage*> items_;
};

#endif
