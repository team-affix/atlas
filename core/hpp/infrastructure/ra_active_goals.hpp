#ifndef RA_ACTIVE_GOALS_HPP
#define RA_ACTIVE_GOALS_HPP

#include <unordered_map>
#include <vector>
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

struct ra_active_goals {
    void insert_active_goal(const goal_lineage*);
    void erase_active_goal(const goal_lineage*);
    bool is_active_goal(const goal_lineage*) const;
    size_t active_goals_size() const;
    bool empty() const;
    void clear_active_goals();
    const goal_lineage* select(size_t index) const;
private:
    std::unordered_map<const goal_lineage*, size_t> index_;
    std::vector<const goal_lineage*> items_;
};

#endif
