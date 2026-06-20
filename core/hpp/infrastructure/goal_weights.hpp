#ifndef GOAL_WEIGHTS_HPP
#define GOAL_WEIGHTS_HPP

#include <unordered_map>
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

struct goal_weights {
    double get(const goal_lineage*) const;
    void set(const goal_lineage*, double);
    void erase(const goal_lineage*);
    void clear_goal_weights();
private:
    std::unordered_map<const goal_lineage*, double> weights_;
};

#endif
