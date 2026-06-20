#ifndef UNIT_GOALS_HPP
#define UNIT_GOALS_HPP

#include <optional>
#include <vector>
#include "value_objects/lineage.hpp"

struct unit_goals {
    void push(const goal_lineage*);
    std::optional<const goal_lineage*> pop();
    void clear();
private:
    std::vector<const goal_lineage*> queue_;
};

#endif
