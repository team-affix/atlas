#ifndef AVOIDANCE_HPP
#define AVOIDANCE_HPP

#include <unordered_set>
#include "../value_objects/lineage.hpp"

struct avoidance {
    std::unordered_set<const resolution_lineage*> decisions;
};

#endif
