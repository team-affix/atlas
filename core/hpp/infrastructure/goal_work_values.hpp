#ifndef GOAL_WORK_VALUES_HPP
#define GOAL_WORK_VALUES_HPP

#include <unordered_map>
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

struct goal_work_values {
    double get(const goal_lineage*) const;
    void set(const goal_lineage*, double);
    void erase(const goal_lineage*);
    void clear_goal_work_values();
private:
    std::unordered_map<const goal_lineage*, double> values_;
};

#endif
